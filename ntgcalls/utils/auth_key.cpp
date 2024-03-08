//
// Created by Laky64 on 08/03/2024.
//

#include "auth_key.hpp"

#include "mod_exp_first.hpp"
#include "ntgcalls/exceptions.hpp"
#include "wrtc/utils/bignum.hpp"

namespace ntgcalls {
    bytes::binary AuthKey::CreateAuthKey(const bytes::binary& firstBytes, const bytes::binary& random, const bytes::binary& primeBytes) {
        const auto first = openssl::BigNum(firstBytes);
        const auto prime = openssl::BigNum(primeBytes);
        if (!ModExpFirst::IsGoodModExpFirst(first, prime)) {
            throw InvalidParams("Bad first prime");
        }
        const auto authKey = openssl::BigNum();
        authKey.setModExp(first, openssl::BigNum(random), prime);
        return authKey.getBytes();
    }

    bytes::binary AuthKey::FillData(const bytes::binary& computedAuthKey) {
        const auto computedAuthKeySize = computedAuthKey.size();
        if (computedAuthKeySize > kSize) {
            throw InvalidParams("Invalid auth key size");
        }
        if (computedAuthKeySize < kSize) {
            const auto authKeyBytes = bytes::binary(kSize);
            set_with_const(authKeyBytes.subspan(0, kSize - computedAuthKeySize), 0);
            bytes::copy(authKeyBytes.subspan(kSize - computedAuthKeySize, computedAuthKeySize), computedAuthKey);
            return authKeyBytes;
        }
        return computedAuthKey;
    }

    uint64_t AuthKey::GetFingerprint(const bytes::binary& authKey) {
        if (authKey.size() != kSize) {
            throw InvalidParams("Invalid auth key size");
        }
        const auto hash = authKey.Sha1();
        return static_cast<uint64_t>(hash[19]) << 56 |
               static_cast<uint64_t>(hash[18]) << 48 |
               static_cast<uint64_t>(hash[17]) << 40 |
               static_cast<uint64_t>(hash[16]) << 32 |
               static_cast<uint64_t>(hash[15]) << 24 |
               static_cast<uint64_t>(hash[14]) << 16 |
               static_cast<uint64_t>(hash[13]) << 8 |
               static_cast<uint64_t>(hash[12]);
    }
} // ntgcalls