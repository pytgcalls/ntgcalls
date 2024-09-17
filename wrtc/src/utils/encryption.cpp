//
// Created by Laky64 on 09/03/2024.
//

#include <wrtc/utils/encryption.hpp>

#include <openssl/aes.h>
#include <climits>

namespace openssl {
    bytes::vector Sha256::Digest(const bytes::const_span data) {
        auto bytes = bytes::vector(SHA256_DIGEST_LENGTH);
        SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), reinterpret_cast<unsigned char*>(bytes.data()));
        return bytes;
    }

    std::array<uint8_t, kSha256Size> Sha256::Concat(const bytes::memory_span& first, const bytes::memory_span& second) {
        auto result = std::array<uint8_t, kSha256Size>();
        auto context = SHA256_CTX();
        SHA256_Init(&context);
        SHA256_Update(&context, first.data, first.size);
        SHA256_Update(&context, second.data, second.size);
        SHA256_Final(result.data(), &context);
        return result;
    }

    bytes::vector Sha1::Digest(const bytes::const_span data) {
        auto bytes = bytes::vector(SHA_DIGEST_LENGTH);
        SHA1(reinterpret_cast<const unsigned char*>(data.data()), data.size(), reinterpret_cast<unsigned char*>(bytes.data()));
        return bytes;
    }

    Aes::KeyIv Aes::PrepareKeyIv(const uint8_t* key, const uint8_t* msgKey, const int x) {
        auto result = KeyIv();
        const auto sha256a = Sha256::Concat(
            bytes::memory_span(msgKey, 16),
            bytes::memory_span(key + x, 36)
        );
        const auto sha256b = Sha256::Concat(
            bytes::memory_span(key + 40 + x, 36),
            bytes::memory_span(msgKey, 16)
        );
        const auto aesKey = result.key.data();
        const auto aesIv = result.iv.data();
        memcpy(aesKey, sha256a.data(), 8);
        memcpy(aesKey + 8, sha256b.data() + 8, 16);
        memcpy(aesKey + 8 + 16, sha256a.data() + 24, 8);
        memcpy(aesIv, sha256b.data(), 4);
        memcpy(aesIv + 4, sha256a.data() + 8, 8);
        memcpy(aesIv + 4 + 8, sha256b.data() + 24, 4);
        return result;
    }

    void Aes::ProcessCtr(const bytes::memory_span from, void *to, KeyIv& keyIv) {
        auto aes = AES_KEY();
        AES_set_encrypt_key(keyIv.key.data(), keyIv.key.size() * CHAR_BIT, &aes);
        uint8_t ecountBuf[16] = {};
        uint32_t offsetInBlock = 0;
        AES_ctr128_encrypt(
            static_cast<const unsigned char*>(from.data),
            static_cast<unsigned char*>(to),
            from.size,
            &aes,
            keyIv.iv.data(),
            ecountBuf,
            &offsetInBlock
        );
    }
} // openssl