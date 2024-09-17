//
// Created by Laky64 on 08/03/2024.
//

#include <ntgcalls/signaling/crypto/mod_exp_first.hpp>

#include <ntgcalls/exceptions.hpp>

namespace signaling {

    ModExpFirst::ModExpFirst(const int32_t g, const bytes::const_span p, const bytes::const_span r) {
        if (r.size() != kRandomPowerSize) {
            throw ntgcalls::InvalidParams("Invalid random size");
        }
        const openssl::BigNum prime(p);
        randomPower = bytes::vector(kRandomPowerSize);
        while (true) {
            bytes::set_random(randomPower);
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
        randomPower.clear();
        modexp.clear();
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
} // signaling