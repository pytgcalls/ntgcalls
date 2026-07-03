//
// Created by Lauren on 21/02/25.
//

#pragma once

#include <wrtc/models/frame_data.hpp>
#include <wrtc/utils/binary.hpp>

namespace wrtc::models {

    class Frame {
    public:
        int64_t ssrc;
        bytes::binary data;
        FrameData frame_data;

        Frame(int64_t ssrc, bytes::binary data, FrameData frame_data);
    };

} // wrtc::models
