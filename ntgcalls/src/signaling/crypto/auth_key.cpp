//
// Created by Laky64 on 08/03/2024.
//

#include <ntgcalls/signaling/crypto/auth_key.hpp>

#include <ntgcalls/signaling/crypto/mod_exp_first.hpp>
#include <ntgcalls/exceptions.hpp>
#include <wrtc/utils/bignum.hpp>
#include <wrtc/utils/encryption.hpp>

namespace signaling {
    bytes::vector AuthKey::CreateAuthKey(const bytes::const_span firstBytes, const bytes::const_span random, const bytes::const_span primeBytes) {
        const auto first = openssl::BigNum(firstBytes);
        const auto prime = openssl::BigNum(primeBytes);
        if (!ModExpFirst::IsGoodModExpFirst(first, prime)) {
            throw ntgcalls::InvalidParams("Bad first prime");
        }
        const auto authKey = openssl::BigNum();
        authKey.setModExp(first, openssl::BigNum(random), prime);
        return authKey.getBytes();
    }

    void AuthKey::FillData(RawKey &authKey, const bytes::const_span computedAuthKey) {
        const auto computedAuthKeySize = computedAuthKey.size();
        if (computedAuthKeySize > EncryptionKey::kSize) {
            throw ntgcalls::InvalidParams("Invalid auth key size");
        }
        const auto authKeyBytes = bytes::make_span(authKey);
        if (computedAuthKeySize < EncryptionKey::kSize) {
            bytes::set_with_const(authKeyBytes.subspan(0, EncryptionKey::kSize - computedAuthKeySize), bytes::byte());
            bytes::copy(authKeyBytes.subspan(EncryptionKey::kSize - computedAuthKeySize), computedAuthKey);
        } else {
            bytes::copy(authKeyBytes, computedAuthKey);
        }
    }

    uint64_t AuthKey::Fingerprint(const bytes::const_span authKey) {
        if (authKey.size() != EncryptionKey::kSize) {
            throw ntgcalls::InvalidParams("Invalid auth key size");
        }
        const auto hash = openssl::Sha1::Digest(authKey);
        return static_cast<uint64_t>(hash[19]) << 56 |
            static_cast<uint64_t>(hash[18]) << 48 |
            static_cast<uint64_t>(hash[17]) << 40 |
            static_cast<uint64_t>(hash[16]) << 32 |
            static_cast<uint64_t>(hash[15]) << 24 |
            static_cast<uint64_t>(hash[14]) << 16 |
            static_cast<uint64_t>(hash[13]) << 8 |
            static_cast<uint64_t>(hash[12]);
    }
} // signaling