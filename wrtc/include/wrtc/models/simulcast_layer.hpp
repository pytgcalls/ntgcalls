//
// Created by Lauren on 02/10/24.
//

#pragma once

#include <cstdint>

namespace wrtc::models {

    class SimulcastLayer {
    public:
        uint32_t ssrc = 0;
        uint32_t fid_ssrc = 0;

        SimulcastLayer(uint32_t ssrc, uint32_t fid_ssrc);
    };

} // wrtc::models
