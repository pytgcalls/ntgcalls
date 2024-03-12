//
// Created by Laky64 on 08/03/2024.
//
#pragma once
#include "wrtc/utils/bignum.hpp"


namespace ntgcalls {

class ModExpFirst {
    static constexpr auto kRandomPowerSize = 256;
public:
    bytes::binary randomPower, modexp;

    static bool IsGoodModExpFirst(const openssl::BigNum &modexp, const openssl::BigNum &prime);

    ModExpFirst(int32_t g, const bytes::binary& p, const bytes::binary& r);

    ~ModExpFirst();
};

} // ntgcalls
