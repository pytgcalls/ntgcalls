//
// Created by Laky64 on 12/04/2024.
//

#pragma once
#include <cstdint>

#include <ntgcalls/utils/binding_utils.hpp>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls {
    class DhConfig {
    public:
        int32_t g = 0;
        bytes::vector p;
        bytes::vector random;

        DhConfig(const int32_t &g, const BYTES(bytes::vector) &p, const BYTES(bytes::vector) &random) {
            this -> g = g;
            this -> p = CPP_BYTES(p, bytes::vector);
            this -> random = CPP_BYTES(random, bytes::vector);
        }
    };
} // ntgcalls
