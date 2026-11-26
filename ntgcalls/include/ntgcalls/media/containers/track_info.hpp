//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <cstdint>
#include <wrtc/models/codec.hpp>

namespace ntgcalls::media::containers {
    struct TrackInfo {
        wrtc::models::Codec codec = wrtc::models::Codec::Opus;
        uint8_t channels = 0;
        uint16_t width = 0;
        uint16_t height = 0;
    };
} // ntgcalls::media::containers
