//
// Created by Laky64 on 09/03/2024.
//
#pragma once

#include <wrtc/utils/binary.hpp>
#include <openssl/sha.h>
#include <array>

namespace openssl {
    constexpr auto kSha256Size = static_cast<size_t>(SHA256_DIGEST_LENGTH);
    constexpr auto kSha512Size = static_cast<size_t>(SHA512_DIGEST_LENGTH);

    class Sha256 {
    public:
        static bytes::vector Digest(bytes::const_span data);

        static std::array<uint8_t, kSha256Size> Concat(const bytes::memory_span& first, const bytes::memory_span& second);
    };

    class Sha512 {
    public:
        static std::array<uint8_t, kSha512Size> Digest(bytes::const_span data);
    };

    class Sha1 {
    public:
        static bytes::vector Digest(bytes::const_span data);
    };

    class Hmac {
    public:
        static std::array<uint8_t, kSha256Size> Sha256(bytes::const_span key, bytes::const_span data);

        static std::array<uint8_t, kSha512Size> Sha512(bytes::const_span key, bytes::const_span data);
    };

    class Pbkdf2 {
    public:
        static std::array<uint8_t, kSha512Size> Sha512(bytes::const_span password, bytes::const_span salt, int iterations);
    };

    class AesCbc {
    public:
        static bytes::binary Encrypt(bytes::const_span data, const std::array<uint8_t, 32>& key, std::array<uint8_t, 16> iv);

        static bytes::binary Decrypt(bytes::const_span data, const std::array<uint8_t, 32>& key, std::array<uint8_t, 16> iv);
    };


    class Aes {
    public:
        struct KeyIv {
            std::array<uint8_t, 32> key;
            std::array<uint8_t, 16> iv;
        };

        static KeyIv PrepareKeyIv(const uint8_t* key, const uint8_t* msgKey, int x);

        static void ProcessCtr(bytes::memory_span from, void* to, KeyIv& keyIv);
    };

} // openssl
