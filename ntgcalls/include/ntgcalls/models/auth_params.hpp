//
// Created by Laky64 on 08/03/2024.
//
#pragma once
#include <cstdint>

#include <wrtc/utils/binary.hpp>

namespace ntgcalls {
    struct AuthParams {
        int64_t key_fingerprint = 0;
        bytes::vector g_a_or_b;
    };
} // ntgcalls

