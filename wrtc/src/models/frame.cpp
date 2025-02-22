//
// Created by Laky64 on 21/02/25.
//

#include <wrtc/models/frame.hpp>

namespace wrtc {

    Frame::Frame(
        const int64_t ssrc,
        bytes::binary data,
        const FrameData frameData
    ): ssrc(ssrc), data(std::move(data)), frameData(frameData) {}

} // wrtc