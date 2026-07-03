//
// Created by Lauren on 23/10/24.
//

#pragma once
#include <cstdint>
#include <api/video/video_rotation.h>

namespace wrtc::models {

    class FrameData {
    public:
        int64_t absolute_capture_timestamp_ms;
        webrtc::VideoRotation rotation;
        uint16_t width, height;

        FrameData() = default;

        FrameData(
            int64_t absolute_capture_timestamp_ms,
            webrtc::VideoRotation rotation,
            uint16_t width,
            uint16_t height
        );
    };

} // wrtc::models
