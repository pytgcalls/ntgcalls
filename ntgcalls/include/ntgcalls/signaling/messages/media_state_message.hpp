//
// Created by Laky64 on 22/03/2024.
//

#pragma once

#include <wrtc/utils/binary.hpp>
#include <ntgcalls/signaling/messages/message.hpp>

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

        static std::unique_ptr<MediaStateMessage> deserialize(const bytes::binary& data);

    private:
        static std::string parseVideoState(VideoState state);

        static VideoState parseVideoState(const std::string& state);
    };

} // signaling
