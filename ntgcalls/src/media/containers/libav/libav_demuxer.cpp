//
// Created by Lauren on 27/11/26.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/audio_sink.hpp>
#include <ntgcalls/media/video_sink.hpp>
#include <ntgcalls/media/containers/bitstream.hpp>
#include <ntgcalls/media/containers/libav/libav_demuxer.hpp>
#include <ntgcalls/media/containers/libav/libav_format.hpp>
#include <rtc_base/logging.h>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

extern "C" {
#include <libavutil/display.h>
}

namespace ntgcalls::media::containers {
    LibAvDemuxer::LibAvDemuxer(BaseSink* sink): video_(dynamic_cast<VideoSink*>(sink) != nullptr), video_sink_(dynamic_cast<VideoSink*>(sink)), format_(BaseMediaDescription::Format::Auto) {
        if (video_) {
            if (const auto description = video_sink_->get_config()) {
                format_ = description->effective_format();
            }
        } else if (const auto audio_sink = dynamic_cast<AudioSink*>(sink)) {
            if (const auto description = audio_sink->get_config()) {
                format_ = description->effective_format();
                channels_ = description->channel_count;
                bitrate_ = description->bitrate;
            }
        }
    }

    LibAvDemuxer::~LibAvDemuxer() {
        av_packet_free(&packet_);
        av_frame_free(&frame_);
        avcodec_free_context(&decoder_);
        avformat_close_input(&context_);
        if (io_) {
            av_freep(static_cast<void*>(&io_->buffer));
            avio_context_free(&io_);
        }
    }

    std::optional<DemuxedFrame> LibAvDemuxer::next(ByteSource& source) {
        source_ = &source;
        if (!context_) {
            open();
        }
        if (decoder_) {
            return next_decoded();
        }
        return next_packet();
    }

    int LibAvDemuxer::read_packet(void* opaque, uint8_t* buffer, const int size) {
        const auto demuxer = static_cast<LibAvDemuxer*>(opaque);
        try {
            const auto count = demuxer->source_->read_some(buffer, static_cast<size_t>(size));
            return count == 0 ? AVERROR_EOF : static_cast<int>(count);
        } catch (...) {
            demuxer->error_ = std::current_exception();
            return AVERROR(EIO);
        }
    }

    int64_t LibAvDemuxer::seek_packet(void* opaque, const int64_t offset, const int whence) {
        const auto demuxer = static_cast<LibAvDemuxer*>(opaque);
        try {
            return demuxer->source_->seek(offset, whence & ~AVSEEK_FORCE).value_or(-1);
        } catch (...) {
            demuxer->error_ = std::current_exception();
            return -1;
        }
    }

    void LibAvDemuxer::open() {
        open_container();
        packet_ = av_packet_alloc();
        frame_ = av_frame_alloc();
        if (video_) {
            open_video();
        } else {
            open_audio();
        }
    }

    void LibAvDemuxer::open_container() {
        seekable_ = source_->seek(0, SEEK_CUR).has_value();
        const auto buffer = static_cast<uint8_t*>(av_malloc(kBufferSize));
        io_ = avio_alloc_context(buffer, kBufferSize, 0, this, &read_packet, nullptr, seekable_ ? &seek_packet : nullptr);
        context_ = avformat_alloc_context();
        context_->pb = io_;
        context_->flags |= AVFMT_FLAG_CUSTOM_IO;
        const auto name = LibAvFormat::input_name(format_);
        const AVInputFormat* input = name ? av_find_input_format(name) : nullptr;
        if (const int error = avformat_open_input(&context_, nullptr, input, nullptr); error < 0) {
            fail(error, "Unsupported or invalid container, use MediaSource.SHELL with ffmpeg for other formats");
        }
        if (const int error = avformat_find_stream_info(context_, nullptr); error < 0) {
            fail(error, "Unable to read the container streams");
        }
        const auto type = video_ ? AVMEDIA_TYPE_VIDEO : AVMEDIA_TYPE_AUDIO;
        const int index = av_find_best_stream(context_, type, -1, -1, nullptr, 0);
        if (index < 0) {
            RTC_LOG(LS_ERROR) << "The container has no " << av_get_media_type_string(type) << " stream";
            throw FFmpegError(std::string("The container has no ") + av_get_media_type_string(type) + " stream");
        }
        for (unsigned int i = 0; i < context_->nb_streams; i++) {
            context_->streams[i]->discard = static_cast<int>(i) == index ? AVDISCARD_DEFAULT : AVDISCARD_ALL;
        }
        stream_ = context_->streams[index];
    }

    void LibAvDemuxer::open_video() {
        const auto* parameters = stream_->codecpar;
        const auto codec = video_codec(parameters->codec_id);
        if (!codec) {
            RTC_LOG(LS_ERROR) << "Unsupported video codec: " << avcodec_get_name(parameters->codec_id);
            throw FFmpegError(std::string("Unsupported video codec: ") + avcodec_get_name(parameters->codec_id) + ", use MediaSource.SHELL with ffmpeg");
        }
        codec_ = *codec;
        rotation_ = stream_rotation(stream_);
        if (stream_->avg_frame_rate.num > 0 && stream_->avg_frame_rate.den > 0) {
            video_sink_->set_source_frame_rate(av_q2d(stream_->avg_frame_rate));
        }
        video_sink_->set_source_reordered(parameters->video_delay > 0);
        RTC_LOG(LS_INFO) << "Sending the " << avcodec_get_name(parameters->codec_id) << " video without re-encoding it when it matches the call";
        if (codec_ == wrtc::models::Codec::H264 && parameters->extradata_size >= 7 && parameters->extradata[0] == 1) {
            parse_avcc(parameters->extradata, static_cast<size_t>(parameters->extradata_size));
        } else if (codec_ == wrtc::models::Codec::AV1 && parameters->extradata_size > 4) {
            parameter_sets_.assign(parameters->extradata + 4, parameters->extradata + parameters->extradata_size);
        }
    }

    void LibAvDemuxer::open_audio() {
        const auto* parameters = stream_->codecpar;
        if (parameters->codec_id == AV_CODEC_ID_OPUS) {
            RTC_LOG(LS_INFO) << "Sending the Opus audio without re-encoding it";
            return;
        }
        const auto* codec = avcodec_find_decoder(parameters->codec_id);
        if (!codec) {
            RTC_LOG(LS_ERROR) << "Unsupported audio codec: " << avcodec_get_name(parameters->codec_id);
            throw FFmpegError(std::string("Unsupported audio codec: ") + avcodec_get_name(parameters->codec_id) + ", use MediaSource.SHELL with ffmpeg");
        }
        decoder_ = avcodec_alloc_context3(codec);
        if (!decoder_ || avcodec_parameters_to_context(decoder_, parameters) < 0) {
            throw FFmpegError("Unable to configure the audio decoder");
        }
        if (const int error = avcodec_open2(decoder_, codec, nullptr); error < 0) {
            fail(error, "Unable to open the audio decoder");
        }
        resampler_ = std::make_unique<codecs::PcmResampler>(decoder_->ch_layout, decoder_->sample_fmt, decoder_->sample_rate, channels_, wrtc::audio_factory::OpusPacket::kSampleRate);
        encoder_ = std::make_unique<codecs::OpusPacketEncoder>(channels_, bitrate_);
        RTC_LOG(LS_INFO) << "Decoding the " << avcodec_get_name(parameters->codec_id) << " audio and encoding it to Opus";
    }

    bool LibAvDemuxer::read_stream_packet() {
        while (true) {
            if (const int error = av_read_frame(context_, packet_); error < 0) {
                if (error_) {
                    std::rethrow_exception(error_);
                }
                if (error == AVERROR_EOF) {
                    return false;
                }
                fail(error, seekable_ ? "Unable to read the container" : "Unable to read the container, an MP4 file read from a pipe must be saved with faststart");
            }
            if (packet_->stream_index == stream_->index) {
                return true;
            }
            av_packet_unref(packet_);
        }
    }

    std::optional<DemuxedFrame> LibAvDemuxer::next_packet() {
        if (!read_stream_packet()) {
            return std::nullopt;
        }
        auto frame = video_ ? video_frame() : audio_frame();
        av_packet_unref(packet_);
        return frame;
    }

    DemuxedFrame LibAvDemuxer::video_frame() {
        const bool keyframe = (packet_->flags & AV_PKT_FLAG_KEY) != 0;
        const auto timestamp = timestamp_ns(packet_->dts != AV_NOPTS_VALUE ? packet_->dts : packet_->pts);
        auto payload = convert_payload(packet_->data, static_cast<size_t>(packet_->size), keyframe);
        const auto* parameters = stream_->codecpar;
        return {
            bytes::make_unique_binary(payload.data(), payload.size()),
            payload.size(),
            timestamp,
            packet_duration(),
            {0, rotation_, static_cast<uint16_t>(parameters->width), static_cast<uint16_t>(parameters->height), codec_, keyframe}
        };
    }

    DemuxedFrame LibAvDemuxer::audio_frame() const {
        const auto size = static_cast<size_t>(packet_->size);
        auto duration = packet_duration();
        if (const auto frames = wrtc::audio_factory::OpusPacket::frames(packet_->data, size); !duration && frames > 0) {
            duration = std::chrono::milliseconds(10) * frames;
        }
        DemuxedFrame frame{bytes::make_unique_binary(packet_->data, size), size, std::nullopt, duration, {}};
        frame.frame_data.codec = wrtc::models::Codec::Opus;
        return frame;
    }

    std::optional<DemuxedFrame> LibAvDemuxer::next_decoded() {
        while (pending_.empty() && !drained_) {
            if (read_stream_packet()) {
                while (avcodec_send_packet(decoder_, packet_) == AVERROR(EAGAIN)) {
                    receive_frames();
                }
                av_packet_unref(packet_);
                receive_frames();
                encode_pcm(false);
            } else {
                avcodec_send_packet(decoder_, nullptr);
                receive_frames();
                encode_pcm(true);
                drained_ = true;
            }
        }
        if (pending_.empty()) {
            return std::nullopt;
        }
        auto frame = std::move(pending_.front());
        pending_.pop_front();
        return frame;
    }

    void LibAvDemuxer::receive_frames() {
        while (avcodec_receive_frame(decoder_, frame_) >= 0) {
            resampler_->convert(frame_->extended_data, frame_->nb_samples, pcm_);
            av_frame_unref(frame_);
        }
    }

    void LibAvDemuxer::encode_pcm(const bool all) {
        const size_t block = static_cast<size_t>(codecs::OpusPacketEncoder::kFrameSamples) * channels_;
        if (all) {
            resampler_->flush(pcm_);
            if (const auto remainder = pcm_.size() % block; remainder != 0) {
                pcm_.resize(pcm_.size() + block - remainder, 0);
            }
        }
        size_t offset = 0;
        for (; offset + block <= pcm_.size(); offset += block) {
            const auto packet = encoder_->encode(pcm_.data() + offset);
            DemuxedFrame frame{bytes::make_unique_binary(packet.data(), packet.size()), packet.size(), std::nullopt, std::chrono::milliseconds(20), {}};
            frame.frame_data.codec = wrtc::models::Codec::Opus;
            pending_.push_back(std::move(frame));
        }
        pcm_.erase(pcm_.begin(), pcm_.begin() + static_cast<std::ptrdiff_t>(offset));
    }

    void LibAvDemuxer::fail(const int error, const std::string& message) const {
        if (error_) {
            std::rethrow_exception(error_);
        }
        char reason[AV_ERROR_MAX_STRING_SIZE] = {};
        av_strerror(error, reason, sizeof(reason));
        RTC_LOG(LS_ERROR) << message << " (" << reason << ")";
        throw FFmpegError(message + " (" + reason + ")");
    }

    std::optional<int64_t> LibAvDemuxer::timestamp_ns(const int64_t value) const {
        if (value == AV_NOPTS_VALUE) {
            return std::nullopt;
        }
        return av_rescale_q(value, stream_->time_base, {1, 1000000000});
    }

    std::optional<std::chrono::nanoseconds> LibAvDemuxer::packet_duration() const {
        if (packet_->duration <= 0) {
            return std::nullopt;
        }
        return std::chrono::nanoseconds(av_rescale_q(packet_->duration, stream_->time_base, {1, 1000000000}));
    }

    void LibAvDemuxer::parse_avcc(const uint8_t* data, const size_t size) {
        length_prefixed_ = true;
        nal_length_size_ = (data[4] & 0x03) + 1;
        size_t offset = 5;
        for (int group = 0; group < 2 && offset < size; group++) {
            const size_t count = group == 0 ? data[offset] & 0x1F : data[offset];
            ++offset;
            for (size_t i = 0; i < count && offset + 2 <= size; i++) {
                constexpr uint8_t kStartCode[] = {0, 0, 0, 1};
                const size_t length = data[offset] << 8 | data[offset + 1];
                offset += 2;
                if (offset + length > size) {
                    return;
                }
                parameter_sets_.insert(parameter_sets_.end(), std::begin(kStartCode), std::end(kStartCode));
                parameter_sets_.insert(parameter_sets_.end(), data + offset, data + offset + length);
                offset += length;
            }
        }
    }

    bytes::binary LibAvDemuxer::convert_payload(const uint8_t* data, const size_t size, const bool keyframe) const {
        if (codec_ == wrtc::models::Codec::H264 && length_prefixed_) {
            bytes::binary annex_b;
            annex_b.reserve(size + parameter_sets_.size() + 16);
            size_t offset = 0;
            while (offset + nal_length_size_ <= size) {
                constexpr uint8_t kStartCode[] = {0, 0, 0, 1};
                size_t length = 0;
                for (size_t i = 0; i < nal_length_size_; i++) {
                    length = length << 8 | data[offset + i];
                }
                offset += nal_length_size_;
                if (offset + length > size) {
                    break;
                }
                annex_b.insert(annex_b.end(), std::begin(kStartCode), std::end(kStartCode));
                annex_b.insert(annex_b.end(), data + offset, data + offset + length);
                offset += length;
            }
            if (keyframe && !Bitstream::has_nal_type(annex_b.data(), annex_b.size(), 7)) {
                annex_b.insert(annex_b.begin(), parameter_sets_.begin(), parameter_sets_.end());
            }
            return annex_b;
        }
        bytes::binary payload;
        if (codec_ == wrtc::models::Codec::AV1 && keyframe && !Bitstream::has_obu_type(data, size, 1)) {
            payload = parameter_sets_;
        }
        payload.insert(payload.end(), data, data + size);
        return payload;
    }

    std::optional<wrtc::models::Codec> LibAvDemuxer::video_codec(const AVCodecID id) {
        switch (id) {
        case AV_CODEC_ID_H264:
            return wrtc::models::Codec::H264;
        case AV_CODEC_ID_VP8:
            return wrtc::models::Codec::VP8;
        case AV_CODEC_ID_VP9:
            return wrtc::models::Codec::VP9;
        case AV_CODEC_ID_AV1:
            return wrtc::models::Codec::AV1;
        default:
            return std::nullopt;
        }
    }

    webrtc::VideoRotation LibAvDemuxer::stream_rotation(const AVStream* stream) {
        const auto* side_data = av_packet_side_data_get(stream->codecpar->coded_side_data, stream->codecpar->nb_coded_side_data, AV_PKT_DATA_DISPLAYMATRIX);
        if (!side_data || side_data->size < 9 * sizeof(int32_t)) {
            return webrtc::kVideoRotation_0;
        }
        const double angle = av_display_rotation_get(reinterpret_cast<const int32_t*>(side_data->data));
        if (std::isnan(angle)) {
            return webrtc::kVideoRotation_0;
        }
        switch ((static_cast<int>(std::lround(-angle)) % 360 + 360) % 360) {
        case 90:
            return webrtc::kVideoRotation_90;
        case 180:
            return webrtc::kVideoRotation_180;
        case 270:
            return webrtc::kVideoRotation_270;
        default:
            return webrtc::kVideoRotation_0;
        }
    }
} // ntgcalls::media::containers
