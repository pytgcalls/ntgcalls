//
// Created by Lauren on 08/03/24.
//
#pragma once
#include <cstdint>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::p2p {
    struct AuthParams {
        int64_t key_fingerprint = 0;
        bytes::binary ga_or_gb;
    };
} // ntgcalls

