//
// Created by Lauren on 14/04/25.
//

#include <bitset>
#include <iterator>
#include <rtc_base/checks.h>
#include <rtc_base/base64.h>
#include <wrtc/interfaces/mtproto/audio_streaming_part_internal.hpp>

namespace wrtc::interfaces::mtproto {
    AudioStreamingPartInternal::AudioStreamingPartInternal(bytes::binary &&data, const std::string& container) {
        av_io_context_ = std::make_unique<AVIOContextImpl>(std::move(data));
        frame_ = av_frame_alloc();

        const AVInputFormat* input_format = av_find_input_format(container.c_str());
        if (!input_format) {
            did_read_to_end_ = true;
            return;
        }

        input_format_context_ = avformat_alloc_context();
        if (!input_format_context_) {
            did_read_to_end_ = true;
            return;
        }

        input_format_context_->pb = av_io_context_->get_context();

        if (avformat_open_input(&input_format_context_, "", input_format, nullptr) < 0) {
            avformat_free_context(input_format_context_);
            input_format_context_ = nullptr;
            did_read_to_end_ = true;
            return;
        }

        if (avformat_find_stream_info(input_format_context_, nullptr) < 0) {
            did_read_to_end_ = true;

            avformat_close_input(&input_format_context_);
            input_format_context_ = nullptr;
            return;
        }

        for (int i = 0; i < input_format_context_->nb_streams; i++) {
            const AVStream* in_stream = input_format_context_->streams[i];

            const AVCodecParameters* in_codecpar = in_stream->codecpar;
            if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
                continue;
            }

            audio_codec_parameters_ = avcodec_parameters_alloc();
            avcodec_parameters_copy(audio_codec_parameters_, in_codecpar);

            stream_id_ = i;
            duration_in_milliseconds_ = static_cast<int>(static_cast<double>(in_stream->duration) * av_q2d(in_stream->time_base) * 1000);

            if (in_stream->metadata) {
                const AVDictionaryEntry* entry = av_dict_get(in_stream->metadata, "TG_META", nullptr, 0);
                if (entry && entry->value) {
                    if (std::optional<std::string> result = webrtc::Base64Decode(entry->value, webrtc::Base64DecodeOptions::kForgiving); result.has_value()) {
                        int offset = 0;
                        channel_updates_ = parse_channel_updates(result.value(), offset);
                    }
                }

                uint32_t video_channel_mask = 0;
                entry = av_dict_get(in_stream->metadata, "ACTIVE_MASK", nullptr, 0);
                if (entry && entry->value) {
                    const std::string source_string = entry->value;
                    video_channel_mask = string_to_uint32(source_string);
                }

                std::vector<std::string> endpoint_list;
                entry = av_dict_get(in_stream->metadata, "ENDPOINTS", nullptr, 0);
                if (entry && entry->value) {
                    const std::string source_string = entry->value;
                    std::vector<std::string> elems;
                    split_string(source_string, ' ', std::back_inserter(elems));
                    endpoint_list = elems;
                }

                if (std::bitset<32> video_channels(video_channel_mask); video_channels.count() == endpoint_list.size()) {
                    size_t endpoint_index = 0;
                    for (size_t i2 = 0; i2 < video_channels.size(); i2++) {
                        if (video_channels[i2]) {
                            endpoint_mapping_.insert(std::make_pair(endpoint_list[endpoint_index], static_cast<int32_t>(i2)));
                            endpoint_index++;
                        }
                    }
                }
            }
            break;
        }

        if (stream_id_ == -1) {
            did_read_to_end_ = true;
        }
    }

    AudioStreamingPartInternal::~AudioStreamingPartInternal() {
        if (frame_) {
            av_frame_free(&frame_);
        }
        if (input_format_context_) {
            avformat_close_input(&input_format_context_);
        }
        if (audio_codec_parameters_) {
            avcodec_parameters_free(&audio_codec_parameters_);
        }
        av_io_context_ = nullptr;
    }

    std::map<std::string, int32_t> AudioStreamingPartInternal::get_endpoint_mapping() const {
        return endpoint_mapping_;
    }

    std::vector<AudioStreamingPartInternal::ChannelUpdate> AudioStreamingPartInternal::get_channel_updates() const {
        return channel_updates_;
    }

    int AudioStreamingPartInternal::get_duration_in_milliseconds() const {
        return duration_in_milliseconds_;
    }

    uint32_t AudioStreamingPartInternal::string_to_uint32(const std::string& string) {
        std::stringstream string_stream(string);
        uint32_t value = 0;
        string_stream >> value;
        return value;
    }

    std::optional<uint32_t> AudioStreamingPartInternal::read_int32(const std::string& data, int& offset) {
        if (offset + 4 > data.length()) {
            return std::nullopt;
        }

        int32_t value = 0;
        std::memcpy(&value, data.data() + offset, 4);
        offset += 4;

        return value;
    }

    std::vector<AudioStreamingPartInternal::ChannelUpdate> AudioStreamingPartInternal::parse_channel_updates(const std::string& data, int& offset) {
        std::vector<ChannelUpdate> result;

        if (!read_int32(data, offset)) {
            return {};
        }

        const auto count = read_int32(data, offset);
        if (!count) {
            return {};
        }

        for (int i = 0; i < count.value(); i++) {
            auto frame_index = read_int32(data, offset);
            if (!frame_index) {
                return {};
            }

            auto channel_id = read_int32(data, offset);
            if (!channel_id) {
                return {};
            }

            auto ssrc = read_int32(data, offset);
            if (!ssrc) {
                return {};
            }

            ChannelUpdate update;
            update.frame_index = static_cast<int>(frame_index.value());
            update.id = static_cast<int>(channel_id.value());
            update.ssrc = ssrc.value();
            result.push_back(update);
        }

        return result;
    }

    int16_t AudioStreamingPartInternal::sample_float_to_int16(const float sample) {
        return av_clip_int16(static_cast<int32_t>(lrint(sample*32767)));
    }

    AudioStreamingPartInternal::ReadPcmResult AudioStreamingPartInternal::read_pcm(AudioStreamingPartPersistentDecoder& persistent_decoder, std::vector<int16_t>& out_pcm) {
        if (did_read_to_end_) {
            return {};
        }

        int out_pcm_sample_offset = 0;
        ReadPcmResult result;

        if (pcm_buffer_sample_offset_ >= pcm_buffer_sample_size_) {
            fill_pcm_buffer(persistent_decoder);
        }

        if (out_pcm.size() != 480 * channel_count_) {
            out_pcm.resize(480 * channel_count_);
        }
        int read_samples = 0;
        if (channel_count_ != 0) {
            read_samples = static_cast<int>(out_pcm.size()) / channel_count_;
        }

        while (out_pcm_sample_offset < read_samples) {
            if (pcm_buffer_sample_offset_ >= pcm_buffer_sample_size_) {
                fill_pcm_buffer(persistent_decoder);

                if (pcm_buffer_sample_offset_ >= pcm_buffer_sample_size_) {
                    break;
                }
            }

            if (const int read_from_pcm_buffer_samples = std::min(pcm_buffer_sample_size_ - pcm_buffer_sample_offset_, read_samples - out_pcm_sample_offset); read_from_pcm_buffer_samples != 0) {
                std::copy_n(pcm_buffer_.begin() + pcm_buffer_sample_offset_ * channel_count_, read_from_pcm_buffer_samples * channel_count_, out_pcm.begin() + out_pcm_sample_offset * channel_count_);
                pcm_buffer_sample_offset_ += read_from_pcm_buffer_samples;
                out_pcm_sample_offset += read_from_pcm_buffer_samples;
                result.num_samples += read_from_pcm_buffer_samples;
                read_sample_count_ += read_from_pcm_buffer_samples;
            }
        }

        result.num_channels = channel_count_;

        return result;
    }

    void AudioStreamingPartInternal::fill_pcm_buffer(AudioStreamingPartPersistentDecoder& persistent_decoder) {
        pcm_buffer_sample_size_ = 0;
        pcm_buffer_sample_offset_ = 0;

        if (did_read_to_end_) {
            return;
        }
        if (!input_format_context_) {
            did_read_to_end_ = true;
            return;
        }

        int ret = 0;
        while (true) {
            ret = av_read_frame(input_format_context_, &packet_);
            if (ret < 0) {
                did_read_to_end_ = true;
                return;
            }

            if (packet_.stream_index != stream_id_) {
                av_packet_unref(&packet_);
                continue;
            }

            ret = persistent_decoder.decode(audio_codec_parameters_, input_format_context_->streams[stream_id_]->time_base, packet_, frame_);
            av_packet_unref(&packet_);

            if (ret == AVERROR(EAGAIN)) {
                continue;
            }

            break;
        }

        if (ret != 0) {
            did_read_to_end_ = true;
            return;
        }

        if (channel_count_ == 0) {
            channel_count_ = frame_->ch_layout.nb_channels;
        }

        if (channel_count_ == 0) {
            did_read_to_end_ = true;
            return;
        }

        if (frame_->ch_layout.nb_channels != channel_count_ || frame_->ch_layout.nb_channels > 8) {
            did_read_to_end_ = true;
            return;
        }

        if (pcm_buffer_.size() < frame_->nb_samples * frame_->ch_layout.nb_channels) {
            pcm_buffer_.resize(frame_->nb_samples * frame_->ch_layout.nb_channels);
        }

        switch (frame_->format) {
            case AV_SAMPLE_FMT_S16: {
                std::memcpy(pcm_buffer_.data(), frame_->data[0], frame_->nb_samples * 2 * frame_->ch_layout.nb_channels);
            } break;
            case AV_SAMPLE_FMT_S16P: {
                int16_t* to_pcm = pcm_buffer_.data(); // NOLINT(misc-const-correctness)
                for (int sample = 0; sample < frame_->nb_samples; ++sample) {
                    for (int channel = 0; channel < frame_->ch_layout.nb_channels; ++channel) {
                        const auto* short_channel = reinterpret_cast<int16_t*>(frame_->data[channel]);
                        *to_pcm++ = short_channel[sample];
                    }
                }
            } break;
            case AV_SAMPLE_FMT_FLT: {
                const auto* float_data = reinterpret_cast<float*>(&frame_->data[0]);
                for (int i = 0; i < frame_->nb_samples * frame_->ch_layout.nb_channels; i++) {
                    pcm_buffer_[i] = sample_float_to_int16(float_data[i]);
                }
            } break;
            case AV_SAMPLE_FMT_FLTP: {
                int16_t* to_pcm = pcm_buffer_.data(); // NOLINT(misc-const-correctness)
                for (int sample = 0; sample < frame_->nb_samples; ++sample) {
                    for (int channel = 0; channel < frame_->ch_layout.nb_channels; ++channel) {
                        const auto* float_channel = reinterpret_cast<float*>(frame_->data[channel]);
                        *to_pcm++ = sample_float_to_int16(float_channel[sample]);
                    }
                }
            } break;
            default: {
                RTC_FATAL() << "Unexpected sample_fmt";
            }
        }

        pcm_buffer_sample_size_ = frame_->nb_samples;
        pcm_buffer_sample_offset_ = 0;
    }
} // wrtc::interfaces::mtproto