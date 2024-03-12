//
// Created by Laky64 on 08/03/2024.
//

#include "mod_exp_first.hpp"

#include "../exceptions.hpp"

namespace ntgcalls {

    ModExpFirst::ModExpFirst(const int32_t g, const bytes::binary& p, const bytes::binary& r) {
        if (r.size() != kRandomPowerSize) {
            throw InvalidParams("Invalid random size");
        }
        const openssl::BigNum prime(p);
        randomPower = bytes::binary(kRandomPowerSize);
        while (true) {
            set_random(randomPower);
            for (auto i = 0; i != kRandomPowerSize; ++i) {
                randomPower[i] ^= r[i];
            }
            const auto modexp = openssl::BigNum();
            modexp.setModExp(
                openssl::BigNum(g),
                openssl::BigNum(randomPower),
                prime
            );
            if (IsGoodModExpFirst(modexp, prime)) {
                this->modexp = modexp.getBytes();
                break;
            }
        }
    }

    ModExpFirst::~ModExpFirst() {
        randomPower = nullptr;
        modexp = nullptr;
    }

    bool ModExpFirst::IsGoodModExpFirst(const openssl::BigNum& modexp, const openssl::BigNum& prime) {
        const auto diff = openssl::BigNum();
        diff.setSub(prime, modexp);
        if (modexp.failed() || prime.failed() || diff.failed()) {
            return false;
        }
        if (constexpr auto kMinDiffBitsCount = 2048 - 64; diff.isNegative()
            || diff.bitsSize() < kMinDiffBitsCount
            || modexp.bitsSize() < kMinDiffBitsCount
            || modexp.bytesSize() > kRandomPowerSize) {
            return false;
            }
        return true;
    }
} // ntgcalls