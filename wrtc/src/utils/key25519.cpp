//
// Created by Laky64 on 07/06/26.
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
    Key25519 Key25519::Generate() {
        std::array<uint8_t, 32> seed{};
        bytes::RandomFill(bytes::span(reinterpret_cast<std::byte*>(seed.data()), seed.size()));
        return FromPrivateKey(bytes::view(seed));
    }

    Key25519 Key25519::FromPrivateKey(const bytes::const_span seed) {
        if (seed.size() != 32) {
            throw wrtc::RTCException("Invalid Ed25519 private key length");
        }
        Key25519 result;
        result.withPrivateKey = true;
        std::ranges::copy(seed, reinterpret_cast<std::byte*>(result.privateKey.data()));
        std::array<uint8_t, 64> expandedPrivateKey{};
        ED25519_keypair_from_seed(result.publicKey.data(), expandedPrivateKey.data(), result.privateKey.data());
        return result;
    }

    Key25519 Key25519::FromPublicKey(const bytes::const_span key) {
        if (key.size() != 32) {
            throw wrtc::RTCException("Invalid Ed25519 public key length");
        }
        Key25519 result;
        std::ranges::copy(key, reinterpret_cast<std::byte*>(result.publicKey.data()));
        return result;
    }

    std::array<uint8_t, 32> Key25519::publicKeyBytes() const {
        return publicKey;
    }

    std::array<uint8_t, 32> Key25519::privateKeyBytes() const {
        return privateKey;
    }

    bool Key25519::hasPrivateKey() const {
        return withPrivateKey;
    }

    std::array<uint8_t, 64> Key25519::sign(const bytes::const_span data) const {
        if (!withPrivateKey) {
            throw wrtc::RTCException("Cannot sign without a private key");
        }
        std::array<uint8_t, 32> publicTmp{};
        std::array<uint8_t, 64> expandedPrivateKey{};
        ED25519_keypair_from_seed(publicTmp.data(), expandedPrivateKey.data(), privateKey.data());
        std::array<uint8_t, 64> signature{};
        if (
            ED25519_sign(
                signature.data(),
                reinterpret_cast<const uint8_t*>(data.data()),
                data.size(),
                expandedPrivateKey.data()
            ) != 1
        ) {
            throw wrtc::RTCException("Failed to sign data with Ed25519");
        }
        return signature;
    }

    bool Key25519::Verify(const bytes::const_span publicKey, const bytes::const_span data, const bytes::const_span signature) {
        if (publicKey.size() != 32 || signature.size() != 64) {
            return false;
        }
        return ED25519_verify(
            reinterpret_cast<const uint8_t*>(data.data()),
            data.size(),
            reinterpret_cast<const uint8_t*>(signature.data()),
            reinterpret_cast<const uint8_t*>(publicKey.data())
        ) == 1;
    }

    std::array<uint8_t, 32> Key25519::computeSharedSecret(const bytes::const_span otherPublicKey) const {
        if (!withPrivateKey) {
            throw wrtc::RTCException("Cannot compute a shared secret without a private key");
        }
        if (otherPublicKey.size() != 32) {
            throw wrtc::RTCException("Invalid Ed25519 public key length");
        }
        std::array<uint8_t, 32> peer{};
        std::ranges::copy(otherPublicKey, reinterpret_cast<std::byte*>(peer.data()));
        const auto montgomery = edwardsToMontgomery(peer);

        const auto expanded = Sha512::Digest(bytes::view(privateKey));
        std::array<uint8_t, 32> scalar{};
        std::copy_n(expanded.begin(), 32, scalar.begin());
        scalar[0] &= 248;
        scalar[31] &= 127;
        scalar[31] |= 64;

        std::array<uint8_t, 32> shared{};
        if (X25519(shared.data(), scalar.data(), montgomery.data()) != 1) {
            throw wrtc::RTCException("Failed to compute the X25519 shared secret");
        }
        const auto wrapped = Hmac::Sha512(bytes::view(std::string_view("tde2e_shared_secret")), bytes::view(shared));
        std::array<uint8_t, 32> result{};
        std::copy_n(wrapped.begin(), 32, result.begin());
        return result;
    }

    std::array<uint8_t, 32> Key25519::edwardsToMontgomery(const std::array<uint8_t, 32>& publicKey) {
        auto y = publicKey;
        y[31] &= 0x7f;

        BN_CTX* ctx = BN_CTX_new();
        BIGNUM* p = nullptr;
        BN_hex2bn(&p, "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed");
        BIGNUM* bnY = BN_lebin2bn(y.data(), y.size(), nullptr);
        BIGNUM* numerator = BN_new();
        BIGNUM* denominator = BN_new();
        BIGNUM* u = BN_new();

        BN_mod_add(numerator, bnY, BN_value_one(), p, ctx);
        BN_mod_sub(denominator, BN_value_one(), bnY, p, ctx);
        BIGNUM* inverse = BN_mod_inverse(nullptr, denominator, p, ctx);
        BN_mod_mul(u, numerator, inverse, p, ctx);

        std::array<uint8_t, 32> result{};
        BN_bn2le_padded(result.data(), result.size(), u);

        BN_free(inverse);
        BN_free(u);
        BN_free(denominator);
        BN_free(numerator);
        BN_free(bnY);
        BN_free(p);
        BN_CTX_free(ctx);
        return result;
    }
} // openssl
