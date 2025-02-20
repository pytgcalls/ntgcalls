//
// Created by Laky64 on 23/10/24.
//

#pragma once
#include <cstdint>
#include <api/video/video_rotation.h>

namespace wrtc {

    class FrameData {
    public:
        int64_t absoluteCaptureTimestampMs;
        webrtc::VideoRotation rotation;
        uint16_t width, height;

        FrameData() = default;

        FrameData(
            int64_t absoluteCaptureTimestampMs,
            webrtc::VideoRotation rotation,
            uint16_t width,
            uint16_t height
        );
    };

} // wrtc
