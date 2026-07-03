//
// Created by Lauren on 08/03/24.
//

#include <ntgcalls/signaling/crypto/auth_key.hpp>

#include <ntgcalls/signaling/crypto/mod_exp_first.hpp>
#include <ntgcalls/exceptions.hpp>
#include <wrtc/utils/bignum.hpp>
#include <wrtc/utils/encryption.hpp>

namespace ntgcalls::signaling::crypto {
    bytes::binary AuthKey::create_auth_key(const bytes::const_span first_bytes, const bytes::const_span random, const bytes::const_span prime_bytes) {
        const auto first = openssl::BigNum(first_bytes);
        const auto prime = openssl::BigNum(prime_bytes);
        if (!ModExpFirst::is_good_mod_exp_first(first, prime)) {
            throw InvalidParams("Bad first prime");
        }
        const auto auth_key = openssl::BigNum();
        auth_key.set_mod_exp(first, openssl::BigNum(random), prime);
        return auth_key.get_bytes();
    }

    void AuthKey::fill_data(RawKey &auth_key, const bytes::const_span computed_auth_key) {
        const auto computed_auth_key_size = computed_auth_key.size();
        if (computed_auth_key_size > EncryptionKey::kSize) {
            throw InvalidParams("Invalid auth key size");
        }
        const auto auth_key_bytes = bytes::make_span(auth_key);
        if (computed_auth_key_size < EncryptionKey::kSize) {
            bytes::set_with_const(auth_key_bytes.subspan(0, EncryptionKey::kSize - computed_auth_key_size), bytes::byte());
            bytes::copy(auth_key_bytes.subspan(EncryptionKey::kSize - computed_auth_key_size), computed_auth_key);
        } else {
            bytes::copy(auth_key_bytes, computed_auth_key);
        }
    }

    uint64_t AuthKey::fingerprint(const bytes::const_span auth_key) {
        if (auth_key.size() != EncryptionKey::kSize) {
            throw InvalidParams("Invalid auth key size");
        }
        const auto hash = openssl::Sha1::digest(auth_key);
        return static_cast<uint64_t>(hash[19]) << 56 |
            static_cast<uint64_t>(hash[18]) << 48 |
            static_cast<uint64_t>(hash[17]) << 40 |
            static_cast<uint64_t>(hash[16]) << 32 |
            static_cast<uint64_t>(hash[15]) << 24 |
            static_cast<uint64_t>(hash[14]) << 16 |
            static_cast<uint64_t>(hash[13]) << 8 |
            static_cast<uint64_t>(hash[12]);
    }
} // ntgcalls::signaling::crypto