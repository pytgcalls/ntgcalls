//
// Created by Lauren on 07/06/26.
//

#include <wrtc/utils/key25519.hpp>

#include <algorithm>
#include <string_view>
#include <openssl/bn.h>
#include <openssl/curve25519.h>
#include <wrtc/utils/random.hpp>
#include <wrtc/utils/encryption.hpp>
#include <wrtc/exceptions.hpp>

namespace openssl {
    Key25519 Key25519::generate() {
        bytes::array<32> seed{};
        bytes::random_fill(bytes::span(seed.data(), seed.size()));
        return from_private_key(bytes::view(seed));
    }

    Key25519 Key25519::from_private_key(const bytes::const_span seed) {
        if (seed.size() != 32) {
            throw wrtc::RTCException("Invalid Ed25519 private key length");
        }
        Key25519 result;
        result.with_private_key_ = true;
        std::ranges::copy(seed, result.private_key_.data());
        bytes::array<64> expanded_private_key{};
        ED25519_keypair_from_seed(result.public_key_.data(), expanded_private_key.data(), result.private_key_.data());
        return result;
    }

    Key25519 Key25519::from_public_key(const bytes::const_span key) {
        if (key.size() != 32) {
            throw wrtc::RTCException("Invalid Ed25519 public key length");
        }
        Key25519 result;
        std::ranges::copy(key, result.public_key_.data());
        return result;
    }

    bytes::array<32> Key25519::public_key_bytes() const {
        return public_key_;
    }

    bytes::array<32> Key25519::private_key_bytes() const {
        return private_key_;
    }

    bool Key25519::has_private_key() const {
        return with_private_key_;
    }

    bytes::array<64> Key25519::sign(const bytes::const_span data) const {
        if (!with_private_key_) {
            throw wrtc::RTCException("Cannot sign without a private key");
        }
        bytes::array<32> public_tmp{};
        bytes::array<64> expanded_private_key{};
        ED25519_keypair_from_seed(public_tmp.data(), expanded_private_key.data(), private_key_.data());
        bytes::array<64> signature{};
        if (
            ED25519_sign(
                signature.data(),
                data.data(),
                data.size(),
                expanded_private_key.data()
            ) != 1
        ) {
            throw wrtc::RTCException("Failed to sign data with Ed25519");
        }
        return signature;
    }

    bool Key25519::verify(const bytes::const_span public_key, const bytes::const_span data, const bytes::const_span signature) {
        if (public_key.size() != 32 || signature.size() != 64) {
            return false;
        }
        return ED25519_verify(
                   data.data(),
                   data.size(),
                   signature.data(),
                   public_key.data()
               ) == 1;
    }

    bytes::array<32> Key25519::compute_shared_secret(const bytes::const_span other_public_key) const {
        if (!with_private_key_) {
            throw wrtc::RTCException("Cannot compute a shared secret without a private key");
        }
        if (other_public_key.size() != 32) {
            throw wrtc::RTCException("Invalid Ed25519 public key length");
        }
        bytes::array<32> peer{};
        std::ranges::copy(other_public_key, peer.data());
        const auto montgomery = edwards_to_montgomery(peer);

        const auto expanded = Sha512::digest(bytes::view(private_key_));
        bytes::array<32> scalar{};
        std::copy_n(expanded.begin(), 32, scalar.begin());
        scalar[0] &= 248;
        scalar[31] &= 127;
        scalar[31] |= 64;

        bytes::array<32> shared{};
        if (X25519(shared.data(), scalar.data(), montgomery.data()) != 1) {
            throw wrtc::RTCException("Failed to compute the X25519 shared secret");
        }
        const auto wrapped = Hmac::sha512(bytes::view(std::string_view("tde2e_shared_secret")), bytes::view(shared));
        bytes::array<32> result{};
        std::copy_n(wrapped.begin(), 32, result.begin());
        return result;
    }

    bytes::array<32> Key25519::edwards_to_montgomery(const bytes::array<32>& public_key) {
        auto y = public_key;
        y[31] &= 0x7f;

        BN_CTX* ctx = BN_CTX_new();
        BIGNUM* p = nullptr;
        BN_hex2bn(&p, "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed");
        BIGNUM* bn_y = BN_lebin2bn(y.data(), y.size(), nullptr);
        BIGNUM* numerator = BN_new();
        BIGNUM* denominator = BN_new();
        BIGNUM* u = BN_new();

        BN_mod_add(numerator, bn_y, BN_value_one(), p, ctx);
        BN_mod_sub(denominator, BN_value_one(), bn_y, p, ctx);
        BIGNUM* inverse = BN_mod_inverse(nullptr, denominator, p, ctx);
        BN_mod_mul(u, numerator, inverse, p, ctx);

        bytes::array<32> result{};
        BN_bn2le_padded(result.data(), result.size(), u);

        BN_free(inverse);
        BN_free(u);
        BN_free(denominator);
        BN_free(numerator);
        BN_free(bn_y);
        BN_free(p);
        BN_CTX_free(ctx);
        return result;
    }
} // openssl
