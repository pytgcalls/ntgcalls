//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <wrtc/utils/binary.hpp>

namespace ntgcalls::media::containers {
    struct MuxedChunk {
        bytes::unique_binary data;
        size_t size = 0;
    };
} // ntgcalls::media::containers
