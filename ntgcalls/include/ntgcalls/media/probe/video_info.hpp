//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <cstdint>
#include <string>

namespace ntgcalls::media {

    struct VideoInfo {
        std::string codec;
        uint16_t width = 0;
        uint16_t height = 0;
        double frame_rate = 0;
        bool b_frames = false;
        bool supported = false;
        bool passthrough = false;
    };

} // ntgcalls::media
