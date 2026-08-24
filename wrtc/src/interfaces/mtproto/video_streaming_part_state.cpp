//
// Created by Lauren on 15/04/25.
//

#include <wrtc/interfaces/mtproto/video_streaming_part_state.hpp>

namespace wrtc::interfaces::mtproto {
    VideoStreamingPartState::VideoStreamingPartState(bytes::binary&& data, const webrtc::MediaType media_type) {
        stream_info_ = consume_stream_info(data);
        if (!stream_info_) {
            return;
        }
        for (size_t i = 0; i < stream_info_->events.size(); i++) {
            if (stream_info_->events[i].offset < 0) {
                continue;
            }
            size_t end_offset = 0;
            if (i == stream_info_->events.size() - 1) {
                end_offset = data.size();
            } else {
                end_offset = stream_info_->events[i + 1].offset;
            }
            if (end_offset <= stream_info_->events[i].offset) {
                continue;
            }
            if (end_offset > data.size()) {
                continue;
            }
            bytes::binary data_slice(data.begin() + stream_info_->events[i].offset, data.begin() + static_cast<ptrdiff_t>(end_offset));
            webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0;
            switch (stream_info_->events[i].rotation) {
            case 0: {
                rotation = webrtc::VideoRotation::kVideoRotation_0;
                break;
            }
            case 90: {
                rotation = webrtc::VideoRotation::kVideoRotation_90;
                break;
            }
            case 180: {
                rotation = webrtc::VideoRotation::kVideoRotation_180;
                break;
            }
            case 270: {
                rotation = webrtc::VideoRotation::kVideoRotation_270;
                break;
            }
            default: {
                break;
            }
            }

            switch (media_type) {
            case webrtc::MediaType::AUDIO: {
                auto part = std::make_unique<AudioStreamingPart>(std::move(data_slice), stream_info_->container, true);
                parsed_audio_parts_.push_back(std::move(part));
            } break;
            case webrtc::MediaType::VIDEO: {
                auto part = std::make_unique<VideoStreamingPartInternal>(stream_info_->events[i].endpoint_id, rotation, std::move(data_slice), stream_info_->container);
                parsed_video_parts_.push_back(std::move(part));
            } break;
            default: {
                break;
            }
            }
        }
    }

    VideoStreamingPartState::~VideoStreamingPartState() {
        parsed_audio_parts_.clear();
        parsed_video_parts_.clear();
        available_frames_.clear();
        stream_info_ = std::nullopt;
    }

    std::optional<media::VideoStreamingPartFrame> VideoStreamingPartState::get_frame_at_relative_timestamp(VideoStreamingSharedState* shared_state, const double timestamp) {
        while (true) {
            while (available_frames_.size() >= 2) {
                if (timestamp >= available_frames_[1].pts) {
                    available_frames_.erase(available_frames_.begin());
                } else {
                    break;
                }
            }

            if (available_frames_.size() < 2) {
                if (!parsed_video_parts_.empty()) {
                    if (auto result = parsed_video_parts_[0]->get_next_frame(shared_state)) {
                        available_frames_.push_back(result.value());
                    } else {
                        parsed_video_parts_.erase(parsed_video_parts_.begin());
                    }
                    continue;
                }
            }

            if (!available_frames_.empty()) {
                for (size_t i = 1; i < available_frames_.size(); i++) {
                    if (timestamp < available_frames_[i].pts) {
                        return available_frames_[i - 1];
                    }
                }
                return available_frames_[available_frames_.size() - 1];
            }
            return std::nullopt;
        }
    }

    std::optional<std::string> VideoStreamingPartState::get_active_endpoint_id() const {
        if (!parsed_video_parts_.empty()) {
            return parsed_video_parts_[0]->get_endpoint_id();
        }
        return std::nullopt;
    }

    bool VideoStreamingPartState::has_remaining_frames() const {
        return !parsed_video_parts_.empty();
    }

    std::vector<AudioStreamingPartState::Channel> VideoStreamingPartState::get_audio10ms_per_channel(AudioStreamingPartPersistentDecoder& persistent_decoder) {
        while (!parsed_audio_parts_.empty()) {
            if (auto first_part_result = parsed_audio_parts_[0]->get_10ms_per_channel(persistent_decoder); first_part_result.empty()) {
                parsed_audio_parts_.erase(parsed_audio_parts_.begin());
            } else {
                return first_part_result;
            }
        }
        return {};
    }

    std::optional<int32_t> VideoStreamingPartState::read_int32(const bytes::binary& data, int& offset) {
        if (offset + 4 > data.size()) {
            return std::nullopt;
        }
        int32_t value = 0;
        std::memcpy(&value, data.data() + offset, 4);
        offset += 4;
        return value;
    }

    std::optional<uint8_t> VideoStreamingPartState::read_bytes_as_int32(const bytes::binary& data, int& offset, const int count) {
        if (offset + count > data.size()) {
            return std::nullopt;
        }

        int32_t value = 0;
        std::memcpy(&value, data.data() + offset, count);
        offset += count;
        return value;
    }

    int32_t VideoStreamingPartState::round_up(const int32_t num_to_round) {
        const int32_t remainder = num_to_round % 4;
        if (remainder == 0) {
            return num_to_round;
        }
        return num_to_round + 4 - remainder;
    }

    std::optional<std::string> VideoStreamingPartState::read_serialized_string(const bytes::binary& data, int& offset) {
        if (const auto tmp = read_bytes_as_int32(data, offset, 1)) {
            int padding_bytes = 0;
            int length = 0;
            if (tmp.value() == 254) {
                if (const auto len = read_bytes_as_int32(data, offset, 3)) {
                    length = len.value();
                    padding_bytes = round_up(length) - length;
                } else {
                    return std::nullopt;
                }
            } else {
                length = tmp.value();
                padding_bytes = round_up(length + 1) - (length + 1);
            }

            if (offset + length > data.size()) {
                return std::nullopt;
            }

            std::string result(data.data() + offset, data.data() + offset + length);

            offset += length;
            offset += padding_bytes;

            return result;
        }
        return std::nullopt;
    }

    std::optional<VideoStreamingPartState::StreamEvent> VideoStreamingPartState::read_video_stream_event(const bytes::binary& data, int& offset) {
        StreamEvent event;

        if (const auto offset_value = read_int32(data, offset)) {
            event.offset = offset_value.value();
        } else {
            return std::nullopt;
        }

        if (const auto endpoint_id = read_serialized_string(data, offset)) {
            event.endpoint_id = endpoint_id.value();
        } else {
            return std::nullopt;
        }

        if (const auto rotation = read_int32(data, offset)) {
            event.rotation = rotation.value();
        } else {
            return std::nullopt;
        }

        if (const auto extra = read_int32(data, offset)) {
            event.extra = extra.value();
        } else {
            return std::nullopt;
        }

        return event;
    }

    std::optional<VideoStreamingPartState::StreamInfo> VideoStreamingPartState::consume_stream_info(bytes::binary& data) {
        int offset = 0;
        if (const auto signature = read_int32(data, offset)) {
            if (signature.value() != 0xa12e810d) {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }

        StreamInfo info;

        if (const auto container = read_serialized_string(data, offset)) {
            info.container = container.value();
        } else {
            return std::nullopt;
        }

        if (const auto active_mask = read_int32(data, offset)) {
            info.active_mask = active_mask.value();
        } else {
            return std::nullopt;
        }

        if (const auto event_count = read_int32(data, offset)) {
            if (event_count > 0) {
                if (const auto event = read_video_stream_event(data, offset)) {
                    info.events.push_back(event.value());
                } else {
                    return std::nullopt;
                }
            } else {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }
        data.erase(data.begin(), data.begin() + offset);
        return info;
    }
} // wrtc::interfaces::mtproto
