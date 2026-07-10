//
// Created by Lauren on 22/03/24.
//

#pragma once

#include <ntgcalls/signaling/messages/message.hpp>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::signaling::messages {
    class MediaStateMessage final : public Message {
    public:
        enum class VideoState {
            Inactive,
            Suspended,
            Active
        };

        enum class VideoRotation {
            Rotation0 = 0,
            Rotation90 = 90,
            Rotation180 = 180,
            Rotation270 = 270
        };

        bool is_muted = false;
        VideoState video_state = VideoState::Inactive;
        VideoRotation video_rotation = VideoRotation::Rotation0;
        VideoState screencast_state = VideoState::Inactive;
        bool is_battery_low = false;

        [[nodiscard]] bytes::binary serialize() const override;

        static std::unique_ptr<MediaStateMessage> deserialize(const bytes::binary& data);

    private:
        static std::string parse_video_state(VideoState state);

        static VideoState parse_video_state(const std::string& state);
    };

} // ntgcalls::signaling::messages
