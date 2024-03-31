//
// Created by Laky64 on 22/03/2024.
//

#pragma once

#include "message.hpp"
#include "wrtc/utils/binary.hpp"

namespace signaling {
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
        bool isMuted = false;
        VideoState videoState = VideoState::Inactive;
        VideoRotation videoRotation = VideoRotation::Rotation0;
        VideoState screencastState = VideoState::Inactive;
        bool isBatteryLow = false;

        [[nodiscard]] bytes::binary serialize() const override;

    private:
        static std::string parseVideoState(VideoState state);
    };

} // signaling
