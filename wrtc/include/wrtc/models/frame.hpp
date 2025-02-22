//
// Created by Laky64 on 21/02/25.
//

#pragma once

#include <wrtc/utils/binary.hpp>
#include <wrtc/models/frame_data.hpp>

namespace wrtc {

    class Frame {
    public:
        int64_t ssrc;
        bytes::binary data;
        FrameData frameData;

        Frame(int64_t ssrc, bytes::binary data, FrameData frameData);
    };

} // wrtc
