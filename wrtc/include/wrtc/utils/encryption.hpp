//
// Created by Lauren on 09/03/24.
//
#pragma once

#include <array>
#include <openssl/sha.h>
#include <wrtc/utils/binary.hpp>

namespace openssl {
    constexpr auto kSha256Size = static_cast<size_t>(SHA256_DIGEST_LENGTH);
    constexpr auto kSha512Size = static_cast<size_t>(SHA512_DIGEST_LENGTH);

    class Sha256 {
    public:
        static bytes::binary digest(bytes::const_span data);

        static bytes::array<kSha256Size> concat(const bytes::memory_span& first, const bytes::memory_span& second);
    };

    class Sha512 {
    public:
        static bytes::array<kSha512Size> digest(bytes::const_span data);
    };

    class Sha1 {
    public:
        static bytes::binary digest(bytes::const_span data);
    };

    class Hmac {
    public:
        static bytes::array<kSha256Size> sha256(bytes::const_span key, bytes::const_span data);

        static bytes::array<kSha512Size> sha512(bytes::const_span key, bytes::const_span data);
    };

    class Pbkdf2 {
    public:
        static bytes::array<kSha512Size> sha512(bytes::const_span password, bytes::const_span salt, int iterations);
    };

    class AesCbc {
    public:
        static bytes::binary encrypt(bytes::const_span data, const bytes::array<32>& key, bytes::array<16> iv);

        static bytes::binary decrypt(bytes::const_span data, const bytes::array<32>& key, bytes::array<16> iv);
    };


    class Aes {
    public:
        struct KeyIv {
            bytes::array<32> key;
            bytes::array<16> iv;
        };

        static KeyIv prepare_key_iv(const bytes::byte* key, const bytes::byte* msg_key, int x);

        static void process_ctr(bytes::memory_span from, void* to, KeyIv& key_iv);
    };

} // openssl
