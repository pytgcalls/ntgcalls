//
// Created by Lauren on 20/02/25.
//

#include <wrtc/models/frame_data.hpp>

namespace wrtc::models {

    FrameData::FrameData(
        const int64_t absolute_capture_timestamp_ms,
        const webrtc::VideoRotation rotation,
        const uint16_t width,
        const uint16_t height
    ): absolute_capture_timestamp_ms(absolute_capture_timestamp_ms), rotation(rotation), width(width), height(height) {}

} // wrtc::models
