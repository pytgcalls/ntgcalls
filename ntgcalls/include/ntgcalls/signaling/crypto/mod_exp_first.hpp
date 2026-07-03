//
// Created by Lauren on 08/03/24.
//
#pragma once
#include <wrtc/utils/bignum.hpp>

namespace ntgcalls::signaling::crypto {
    class ModExpFirst {
        static constexpr size_t kRandomPowerSize = 256;
    public:
        bytes::binary random_power, modexp;

        static bool is_good_mod_exp_first(const openssl::BigNum &modexp, const openssl::BigNum &prime);

        ModExpFirst(int32_t g, bytes::const_span p, bytes::const_span r);

        ~ModExpFirst();
    };
} // ntgcalls::signaling::crypto
