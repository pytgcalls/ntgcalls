//
// Created by Laky64 on 20/02/25.
//

#include <wrtc/models/frame_data.hpp>

namespace wrtc {

    FrameData::FrameData(
        const int64_t absoluteCaptureTimestampMs,
        const webrtc::VideoRotation rotation,
        const uint16_t width,
        const uint16_t height
    ): absoluteCaptureTimestampMs(absoluteCaptureTimestampMs), rotation(rotation), width(width), height(height) {}

} // ntgcalls