//
// Created by Laky64 on 09/03/2024.
//
#pragma once

#include <wrtc/utils/binary.hpp>
#include <openssl/sha.h>
#include <array>

namespace openssl {
    constexpr auto kSha256Size = static_cast<size_t>(SHA256_DIGEST_LENGTH);

    class Sha256 {
    public:
        static bytes::vector Digest(bytes::const_span data);

        static std::array<uint8_t, kSha256Size> Concat(const bytes::memory_span& first, const bytes::memory_span& second);
    };

    class Sha1 {
    public:
        static bytes::vector Digest(bytes::const_span data);
    };


    class Aes {
    public:
        struct KeyIv {
            std::array<uint8_t, 32> key;
            std::array<uint8_t, 16> iv;
        };

        static KeyIv PrepareKeyIv(const uint8_t* key, const uint8_t* msgKey, int x);

        static void ProcessCtr(bytes::memory_span from, void *to, KeyIv& keyIv);
    };

} // openssl
