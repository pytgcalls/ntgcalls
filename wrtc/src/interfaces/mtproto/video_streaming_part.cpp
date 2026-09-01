//
// Created by Lauren on 15/04/25.
//

#include <wrtc/interfaces/mtproto/video_streaming_part.hpp>

namespace wrtc::interfaces::mtproto {
    VideoStreamingPart::VideoStreamingPart(bytes::binary&& data, webrtc::MediaType media_type) {
        if (!data.empty()) {
            state_ = std::make_unique<VideoStreamingPartState>(std::move(data), media_type);
        }
    }

    VideoStreamingPart::~VideoStreamingPart() {
        state_ = nullptr;
    }

    std::optional<std::string> VideoStreamingPart::get_active_endpoint_id() const {
        return state_ ? state_->get_active_endpoint_id() : std::nullopt;
    }

    std::optional<media::VideoStreamingPartFrame> VideoStreamingPart::get_frame_at_relative_timestamp(VideoStreamingSharedState* shared_state, const double timestamp) const {
        return state_ ? state_->get_frame_at_relative_timestamp(shared_state, timestamp) : std::nullopt;
    }

    std::vector<AudioStreamingPartState::Channel> VideoStreamingPart::get_audio10ms_per_channel(AudioStreamingPartPersistentDecoder& persistent_decoder) const {
        return state_ ? state_->get_audio10ms_per_channel(persistent_decoder) : std::vector<AudioStreamingPartState::Channel>();
    }
} // wrtc::interfaces::mtproto
