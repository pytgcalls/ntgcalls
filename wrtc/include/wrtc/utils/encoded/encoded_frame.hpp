//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <wrtc/models/frame_data.hpp>
#include <wrtc/utils/binary.hpp>

namespace wrtc::utils::encoded {
    struct EncodedFrame {
        bytes::binary payload;
        models::FrameData frame_data;
    };
} // wrtc::utils::encoded
