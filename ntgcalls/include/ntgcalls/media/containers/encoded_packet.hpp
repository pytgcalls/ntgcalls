//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <chrono>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::media::containers {
    struct EncodedPacket {
        bytes::unique_binary data;
        size_t size = 0;
        int64_t timestamp_ms = 0;
        std::chrono::microseconds duration{0};
        bool keyframe = true;
    };
} // ntgcalls::media::containers
