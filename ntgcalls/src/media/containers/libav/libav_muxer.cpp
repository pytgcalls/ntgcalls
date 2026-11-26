//
// Created by Lauren on 27/11/26.
//

#include <algorithm>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/containers/bitstream.hpp>
#include <ntgcalls/media/containers/libav/libav_format.hpp>
#include <ntgcalls/media/containers/libav/libav_muxer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::media::containers {
    template<BaseMediaDescription::Format Format>
    LibAvMuxer<Format>::LibAvMuxer(const TrackInfo& track): track_(track), time_base_({1, 1000}) {
        if (avformat_alloc_output_context2(&context_, output_format(), nullptr, nullptr) < 0 || !context_) {
            throw FFmpegError("Unable to create the output container");
        }
        const auto buffer = static_cast<uint8_t*>(av_malloc(kBufferSize));
        io_ = avio_alloc_context(buffer, kBufferSize, 1, this, nullptr, &write_packet, nullptr);
        context_->pb = io_;
        context_->flags |= AVFMT_FLAG_CUSTOM_IO;
        packet_ = av_packet_alloc();
    }

    template<BaseMediaDescription::Format Format>
    LibAvMuxer<Format>::~LibAvMuxer() {
        av_packet_free(&packet_);
        av_frame_free(&frame_);
        avcodec_free_context(&encoder_);
        avformat_free_context(context_);
        if (io_) {
            av_freep(static_cast<void*>(&io_->buffer));
            avio_context_free(&io_);
        }
    }

    template<BaseMediaDescription::Format Format>
    int LibAvMuxer<Format>::write_packet(void* opaque, const uint8_t* buffer, const int size) {
        try {
            auto& output = static_cast<LibAvMuxer*>(opaque)->output_;
            output.insert(output.end(), buffer, buffer + size);
            return size;
        } catch (...) {
            return AVERROR(ENOMEM);
        }
    }

    template<BaseMediaDescription::Format Format>
    const AVOutputFormat* LibAvMuxer<Format>::output_format() {
        return av_guess_format(LibAvFormat::output_name(Format), nullptr, nullptr);
    }

    template<BaseMediaDescription::Format Format>
    AVCodecID LibAvMuxer<Format>::codec_id(const wrtc::models::Codec codec) {
        switch (codec) {
        case wrtc::models::Codec::Opus:
            return AV_CODEC_ID_OPUS;
        case wrtc::models::Codec::H264:
            return AV_CODEC_ID_H264;
        case wrtc::models::Codec::VP8:
            return AV_CODEC_ID_VP8;
        case wrtc::models::Codec::VP9:
            return AV_CODEC_ID_VP9;
        case wrtc::models::Codec::AV1:
            return AV_CODEC_ID_AV1;
        case wrtc::models::Codec::PCM:
            return AV_CODEC_ID_PCM_S16LE;
        default:
            return AV_CODEC_ID_NONE;
        }
    }

    template<BaseMediaDescription::Format Format>
    MuxedChunk LibAvMuxer<Format>::add(const EncodedPacket packet) {
        const bool video = track_.width > 0 && track_.height > 0;
        if (!header_written_) {
            if (video && !packet.keyframe) {
                return {};
            }
            write_header(packet);
            first_timestamp_ = packet.timestamp_ms;
        }
        if (video) {
            const auto pts = std::max(packet.timestamp_ms - first_timestamp_, next_pts_);
            next_pts_ = pts;
            write(packet.data.get(), packet.size, pts, 0, packet.keyframe);
        } else if (encoder_) {
            const auto* samples = reinterpret_cast<const int16_t*>(packet.data.get());
            const auto count = static_cast<std::ptrdiff_t>(packet.size / sizeof(int16_t));
            pcm_.insert(pcm_.end(), samples, samples + count);
            encode_pcm(false);
        } else {
            const auto duration = packet.duration.count() * time_base_.den / 1000000;
            write(packet.data.get(), packet.size, next_pts_, duration, true);
            next_pts_ += duration;
        }
        return take_output();
    }

    template<BaseMediaDescription::Format Format>
    MuxedChunk LibAvMuxer<Format>::finish() {
        if (!header_written_) {
            return {};
        }
        if (encoder_) {
            encode_pcm(true);
        }
        av_write_trailer(context_);
        return take_output();
    }

    template<BaseMediaDescription::Format Format>
    void LibAvMuxer<Format>::write_header(const EncodedPacket& packet) {
        stream_ = avformat_new_stream(context_, nullptr);
        if (!stream_) {
            throw FFmpegError("Unable to create the output stream");
        }
        auto* parameters = stream_->codecpar;
        if (track_.width > 0 && track_.height > 0) {
            parameters->codec_type = AVMEDIA_TYPE_VIDEO;
            parameters->codec_id = codec_id(track_.codec);
            parameters->width = track_.width;
            parameters->height = track_.height;
        } else {
            time_base_ = {1, static_cast<int>(track_.sample_rate)};
            if (track_.codec == wrtc::models::Codec::PCM && pcm_encoder() != AV_CODEC_ID_NONE) {
                open_encoder();
            } else {
                parameters->codec_type = AVMEDIA_TYPE_AUDIO;
                parameters->codec_id = codec_id(track_.codec);
                parameters->sample_rate = static_cast<int>(track_.sample_rate);
                av_channel_layout_default(&parameters->ch_layout, track_.channels);
                if (track_.codec == wrtc::models::Codec::PCM) {
                    parameters->format = AV_SAMPLE_FMT_S16;
                    parameters->bits_per_coded_sample = 16;
                    parameters->block_align = 2 * track_.channels;
                    parameters->bit_rate = static_cast<int64_t>(track_.sample_rate) * track_.channels * 16;
                }
            }
        }
        if (const auto data = extradata(track_, packet); !data.empty()) {
            parameters->extradata = static_cast<uint8_t*>(av_mallocz(data.size() + AV_INPUT_BUFFER_PADDING_SIZE));
            std::memcpy(parameters->extradata, data.data(), data.size());
            parameters->extradata_size = static_cast<int>(data.size());
        }
        stream_->time_base = time_base_;
        AVDictionary* options = nullptr;
        if constexpr (Format == BaseMediaDescription::Format::MP4) {
            av_dict_set(&options, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
        }
        const int error = avformat_write_header(context_, &options);
        av_dict_free(&options);
        if (error < 0) {
            char reason[AV_ERROR_MAX_STRING_SIZE] = {};
            av_strerror(error, reason, sizeof(reason));
            RTC_LOG(LS_ERROR) << "Unable to write the container header (" << reason << ")";
            throw FFmpegError(std::string("Unable to write the container header (") + reason + ")");
        }
        header_written_ = true;
    }

    template<BaseMediaDescription::Format Format>
    void LibAvMuxer<Format>::open_encoder() {
        const auto* codec = avcodec_find_encoder(pcm_encoder());
        encoder_ = codec ? avcodec_alloc_context3(codec) : nullptr;
        if (!encoder_) {
            RTC_LOG(LS_ERROR) << "The " << avcodec_get_name(pcm_encoder()) << " encoder is not available";
            throw FFmpegError(std::string("The ") + avcodec_get_name(pcm_encoder()) + " encoder is not available");
        }
        encoder_->sample_fmt = sample_format(codec);
        encoder_->sample_rate = static_cast<int>(track_.sample_rate);
        encoder_->time_base = time_base_;
        encoder_->bit_rate = kBitratePerChannel * track_.channels;
        av_channel_layout_default(&encoder_->ch_layout, track_.channels);
        if (context_->oformat->flags & AVFMT_GLOBALHEADER) {
            encoder_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        if (avcodec_open2(encoder_, codec, nullptr) < 0) {
            RTC_LOG(LS_ERROR) << "Unable to open the " << codec->name << " encoder";
            throw FFmpegError(std::string("Unable to open the ") + codec->name + " encoder");
        }
        frame_ = av_frame_alloc();
        frame_->format = encoder_->sample_fmt;
        frame_->sample_rate = encoder_->sample_rate;
        frame_->nb_samples = encoder_->frame_size > 0 ? encoder_->frame_size : kDefaultFrameSamples;
        av_channel_layout_copy(&frame_->ch_layout, &encoder_->ch_layout);
        if (av_frame_get_buffer(frame_, 0) < 0) {
            throw FFmpegError("Unable to allocate the audio frame");
        }
        avcodec_parameters_from_context(stream_->codecpar, encoder_);
    }

    template<BaseMediaDescription::Format Format>
    AVSampleFormat LibAvMuxer<Format>::sample_format(const AVCodec* codec) {
        const void* formats = nullptr;
        int count = 0;
        if (avcodec_get_supported_config(nullptr, codec, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, &formats, &count) < 0 || !formats || count == 0) {
            return AV_SAMPLE_FMT_S16;
        }
        const auto* supported = static_cast<const AVSampleFormat*>(formats);
        for (int i = 0; i < count; i++) {
            if (supported[i] == AV_SAMPLE_FMT_S16) {
                return AV_SAMPLE_FMT_S16;
            }
        }
        return supported[0];
    }

    template<BaseMediaDescription::Format Format>
    void LibAvMuxer<Format>::write(const uint8_t* data, const size_t size, const int64_t pts, const int64_t duration, const bool keyframe) {
        if (av_new_packet(packet_, static_cast<int>(size)) < 0) {
            return;
        }
        std::memcpy(packet_->data, data, size);
        packet_->pts = pts;
        packet_->dts = pts;
        packet_->duration = duration;
        packet_->flags = keyframe ? AV_PKT_FLAG_KEY : 0;
        packet_->stream_index = stream_->index;
        av_packet_rescale_ts(packet_, time_base_, stream_->time_base);
        av_write_frame(context_, packet_);
        av_packet_unref(packet_);
    }

    template<BaseMediaDescription::Format Format>
    void LibAvMuxer<Format>::encode_pcm(const bool all) {
        const auto channels = static_cast<size_t>(track_.channels);
        const auto frame_samples = static_cast<size_t>(frame_->nb_samples);
        const auto block = frame_samples * channels;
        const bool small_last_frame = encoder_->codec->capabilities & (AV_CODEC_CAP_SMALL_LAST_FRAME | AV_CODEC_CAP_VARIABLE_FRAME_SIZE);
        if (all && !small_last_frame && pcm_.size() % block != 0) {
            pcm_.resize(pcm_.size() + block - pcm_.size() % block, 0);
        }
        while (pcm_.size() >= block || (all && !pcm_.empty())) {
            const auto count = std::min(block, pcm_.size());
            if (av_frame_make_writable(frame_) < 0) {
                return;
            }
            frame_->nb_samples = static_cast<int>(count / channels);
            fill_frame(pcm_.data(), frame_->nb_samples);
            frame_->pts = next_pts_;
            next_pts_ += frame_->nb_samples;
            pcm_.erase(pcm_.begin(), pcm_.begin() + static_cast<std::ptrdiff_t>(count));
            if (avcodec_send_frame(encoder_, frame_) >= 0) {
                drain_encoder();
            }
        }
        if (all) {
            avcodec_send_frame(encoder_, nullptr);
            drain_encoder();
        }
    }

    template<BaseMediaDescription::Format Format>
    void LibAvMuxer<Format>::fill_frame(const int16_t* pcm, const int samples) const {
        const int channels = track_.channels;
        const auto format = static_cast<AVSampleFormat>(frame_->format);
        const bool planar = av_sample_fmt_is_planar(format);
        for (int i = 0; i < samples; i++) {
            for (int c = 0; c < channels; c++) {
                const auto sample = pcm[i * channels + c];
                const int plane = planar ? c : 0;
                const int index = planar ? i : i * channels + c;
                if (format == AV_SAMPLE_FMT_FLT || format == AV_SAMPLE_FMT_FLTP) {
                    reinterpret_cast<float*>(frame_->data[plane])[index] = static_cast<float>(sample) / 32768.0f;
                } else {
                    reinterpret_cast<int16_t*>(frame_->data[plane])[index] = sample;
                }
            }
        }
    }

    template<BaseMediaDescription::Format Format>
    void LibAvMuxer<Format>::drain_encoder() const {
        while (avcodec_receive_packet(encoder_, packet_) >= 0) {
            packet_->stream_index = stream_->index;
            av_packet_rescale_ts(packet_, encoder_->time_base, stream_->time_base);
            av_write_frame(context_, packet_);
            av_packet_unref(packet_);
        }
    }

    template<BaseMediaDescription::Format Format>
    MuxedChunk LibAvMuxer<Format>::take_output() {
        avio_flush(io_);
        if (output_.empty()) {
            return {};
        }
        MuxedChunk chunk{bytes::make_unique_binary(output_.data(), output_.size()), output_.size()};
        output_.clear();
        return chunk;
    }

    template<BaseMediaDescription::Format Format>
    bytes::binary LibAvMuxer<Format>::extradata(const TrackInfo& track, const EncodedPacket& packet) {
        if (track.codec == wrtc::models::Codec::Opus) {
            return {'O', 'p', 'u', 's', 'H', 'e', 'a', 'd', 1, track.channels, 0x38, 0x01, 0x80, 0xBB, 0x00, 0x00, 0, 0, 0};
        }
        if (track.codec == wrtc::models::Codec::H264) {
            return Bitstream::h264_parameter_sets(packet.data.get(), packet.size);
        }
        if (track.codec == wrtc::models::Codec::AV1) {
            return Bitstream::av1_sequence_header(packet.data.get(), packet.size);
        }
        return {};
    }

    template class LibAvMuxer<BaseMediaDescription::Format::Ogg>;
    template class LibAvMuxer<BaseMediaDescription::Format::Matroska>;
    template class LibAvMuxer<BaseMediaDescription::Format::Webm>;
    template class LibAvMuxer<BaseMediaDescription::Format::MP4>;
    template class LibAvMuxer<BaseMediaDescription::Format::WAV>;
    template class LibAvMuxer<BaseMediaDescription::Format::FLAC>;
} // ntgcalls::media::containers
