//
// Created by Lauren on 15/04/25.
//

#pragma once
#include <wrtc/interfaces/mtproto/video_streaming_part_state.hpp>

namespace wrtc::interfaces::mtproto {

    class VideoStreamingPart {
        std::unique_ptr<VideoStreamingPartState> state_;

    public:
        explicit VideoStreamingPart(bytes::binary&& data, webrtc::MediaType media_type = webrtc::MediaType::VIDEO);

        ~VideoStreamingPart();

        [[nodiscard]] std::optional<std::string> get_active_endpoint_id() const;

        std::optional<media::VideoStreamingPartFrame> get_frame_at_relative_timestamp(VideoStreamingSharedState* shared_state, double timestamp) const;

        std::vector<AudioStreamingPartState::Channel> get_audio10ms_per_channel(AudioStreamingPartPersistentDecoder& persistent_decoder) const;
    };

} // wrtc::interfaces::mtproto
