//
// Created by Lauren on 07/06/26.
//

#pragma once
#include <array>
#include <wrtc/utils/binary.hpp>

namespace openssl {
    class Key25519 {
        bytes::array<32> private_key_{};
        bytes::array<32> public_key_{};
        bool with_private_key_ = false;

        static bytes::array<32> edwards_to_montgomery(const bytes::array<32>& public_key);

    public:
        Key25519() = default;

        static Key25519 generate();

        static Key25519 from_private_key(bytes::const_span seed);

        static Key25519 from_public_key(bytes::const_span key);

        [[nodiscard]] bytes::array<32> public_key_bytes() const;

        [[nodiscard]] bytes::array<32> private_key_bytes() const;

        [[nodiscard]] bool has_private_key() const;

        [[nodiscard]] bytes::array<64> sign(bytes::const_span data) const;

        [[nodiscard]] bytes::array<32> compute_shared_secret(bytes::const_span other_public_key) const;

        static bool verify(bytes::const_span public_key, bytes::const_span data, bytes::const_span signature);
    };
} // openssl
