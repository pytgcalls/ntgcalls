//
// Created by Laky64 on 08/03/2024.
//
#pragma once
#include <wrtc/utils/bignum.hpp>


namespace signaling {
    class ModExpFirst {
        static constexpr size_t kRandomPowerSize = 256;
    public:
        bytes::vector randomPower, modexp;

        static bool IsGoodModExpFirst(const openssl::BigNum &modexp, const openssl::BigNum &prime);

        ModExpFirst(int32_t g, bytes::const_span p, bytes::const_span r);

        ~ModExpFirst();
    };
} // signaling
