//
// Created by Laky64 on 08/03/2024.
//
#pragma once
#include "wrtc/utils/bignum.hpp"


namespace ntgcalls {

class ModExpFirst {
    static constexpr auto kRandomPowerSize = 256;

    static bool IsGoodModExpFirst(const openssl::BigNum &modexp, const openssl::BigNum &prime);

public:
    bytes::binary randomPower, modexp;

    ModExpFirst(int32_t g, const bytes::binary& p, const bytes::binary& r);
};

} // ntgcalls
