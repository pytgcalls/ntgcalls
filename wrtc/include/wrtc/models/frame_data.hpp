//
// Created by Lauren on 23/10/24.
//

#pragma once
#include <cstdint>
#include <api/video/video_rotation.h>
#include <wrtc/models/codec.hpp>

namespace wrtc::models {

    class FrameData {
    public:
        int64_t absolute_capture_timestamp_ms = 0;
        webrtc::VideoRotation rotation = webrtc::kVideoRotation_0;
        uint16_t width = 0, height = 0;
        Codec codec = Codec::PCM;

        FrameData() = default;

        FrameData(
            int64_t absolute_capture_timestamp_ms,
            webrtc::VideoRotation rotation,
            uint16_t width,
            uint16_t height,
            Codec codec
        );
    };

} // wrtc::models
