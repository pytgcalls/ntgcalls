//
// Created by Laky64 on 02/10/24.
//

#pragma once

#include <cstdint>

namespace wrtc {

    class SimulcastLayer {
    public:
        uint32_t ssrc = 0;
        uint32_t fidSsrc = 0;

        SimulcastLayer(uint32_t ssrc, uint32_t fidSsrc);
    };

} // wrtc
