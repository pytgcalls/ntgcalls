//
// Created by Lauren on 21/02/25.
//

#include <wrtc/models/frame.hpp>

namespace wrtc::models {

    Frame::Frame(
        const int64_t ssrc,
        bytes::binary data,
        const FrameData frame_data
    ): ssrc(ssrc), data(std::move(data)), frame_data(frame_data) {}

} // wrtc::models
