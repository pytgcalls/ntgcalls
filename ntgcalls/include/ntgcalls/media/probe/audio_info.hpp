//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <cstdint>
#include <string>

namespace ntgcalls::media {

    struct AudioInfo {
        std::string codec;
        uint32_t sample_rate = 0;
        uint8_t channels = 0;
        bool supported = false;
        bool passthrough = false;
    };

} // ntgcalls::media
