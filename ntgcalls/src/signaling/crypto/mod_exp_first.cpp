//
// Created by Lauren on 08/03/24.
//

#include <ntgcalls/signaling/crypto/mod_exp_first.hpp>

#include <ntgcalls/exceptions.hpp>
#include <wrtc/utils/random.hpp>

namespace ntgcalls::signaling::crypto {

    ModExpFirst::ModExpFirst(const int32_t g, const bytes::const_span p, const bytes::const_span r) {
        if (r.size() != kRandomPowerSize) {
            throw InvalidParams("Invalid random size");
        }
        const openssl::BigNum prime(p);
        random_power = bytes::binary(kRandomPowerSize);
        while (true) {
            bytes::set_random(random_power);
            for (auto i = 0; i != kRandomPowerSize; ++i) {
                random_power[i] ^= r[i];
            }
            const auto m = openssl::BigNum();
            m.set_mod_exp(
                openssl::BigNum(g),
                openssl::BigNum(random_power),
                prime
            );
            if (is_good_mod_exp_first(m, prime)) {
                this->modexp = m.get_bytes();
                break;
            }
        }
    }

    ModExpFirst::~ModExpFirst() {
        random_power.clear();
        modexp.clear();
    }

    bool ModExpFirst::is_good_mod_exp_first(const openssl::BigNum& modexp, const openssl::BigNum& prime) {
        const auto diff = openssl::BigNum();
        diff.set_sub(prime, modexp);
        if (modexp.failed() || prime.failed() || diff.failed()) {
            return false;
        }
        if (constexpr auto kMinDiffBitsCount = 2048 - 64; diff.is_negative()
            || diff.bits_size() < kMinDiffBitsCount
            || modexp.bits_size() < kMinDiffBitsCount
            || modexp.bytes_size() > kRandomPowerSize) {
            return false;
            }
        return true;
    }
} // ntgcalls::signaling::crypto