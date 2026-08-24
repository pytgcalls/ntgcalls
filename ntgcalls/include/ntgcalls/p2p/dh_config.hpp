//
// Created by Lauren on 12/04/24.
//

#pragma once
#include <cstdint>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::p2p {
    class DhConfig {
    public:
        int32_t g = 0;
        bytes::binary p;
        bytes::binary random;

        DhConfig(int32_t g, const bytes::binary& p, const bytes::binary& random) {
            this->g = g;
            this->p = p;
            this->random = random;
        }
    };
} // ntgcalls
