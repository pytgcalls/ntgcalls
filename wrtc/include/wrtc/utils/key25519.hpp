//
// Created by Laky64 on 07/06/26.
//

#pragma once
#include <array>
#include <wrtc/utils/binary.hpp>

namespace openssl {
    class Key25519 {
        std::array<uint8_t, 32> privateKey{};
        std::array<uint8_t, 32> publicKey{};
        bool withPrivateKey = false;

        static std::array<uint8_t, 32> edwardsToMontgomery(const std::array<uint8_t, 32>& publicKey);

    public:
        Key25519() = default;

        static Key25519 Generate();

        static Key25519 FromPrivateKey(bytes::const_span seed);

        static Key25519 FromPublicKey(bytes::const_span key);

        [[nodiscard]] std::array<uint8_t, 32> publicKeyBytes() const;

        [[nodiscard]] std::array<uint8_t, 32> privateKeyBytes() const;

        [[nodiscard]] bool hasPrivateKey() const;

        [[nodiscard]] std::array<uint8_t, 64> sign(bytes::const_span data) const;

        [[nodiscard]] std::array<uint8_t, 32> computeSharedSecret(bytes::const_span otherPublicKey) const;

        static bool Verify(bytes::const_span publicKey, bytes::const_span data, bytes::const_span signature);
    };
} // openssl
