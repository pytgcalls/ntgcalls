//
// Created by Laky64 on 09/03/2024.
//

#include <wrtc/utils/encryption.hpp>

#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <climits>

namespace openssl {
    bytes::vector Sha256::Digest(const bytes::const_span data) {
        auto bytes = bytes::vector(SHA256_DIGEST_LENGTH);
        SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), reinterpret_cast<unsigned char*>(bytes.data()));
        return bytes;
    }

    std::array<uint8_t, kSha512Size> Sha512::Digest(const bytes::const_span data) {
        auto result = std::array<uint8_t, kSha512Size>();
        SHA512(reinterpret_cast<const unsigned char*>(data.data()), data.size(), result.data());
        return result;
    }

    std::array<uint8_t, kSha256Size> Hmac::Sha256(const bytes::const_span key, const bytes::const_span data) {
        auto result = std::array<uint8_t, kSha256Size>();
        unsigned int length = 0;
        HMAC(
            EVP_sha256(),
            key.data(), static_cast<int>(key.size()),
            reinterpret_cast<const unsigned char*>(data.data()), data.size(),
            result.data(), &length
        );
        return result;
    }

    std::array<uint8_t, kSha512Size> Hmac::Sha512(const bytes::const_span key, const bytes::const_span data) {
        auto result = std::array<uint8_t, kSha512Size>();
        unsigned int length = 0;
        HMAC(
            EVP_sha512(),
            key.data(), static_cast<int>(key.size()),
            reinterpret_cast<const unsigned char*>(data.data()), data.size(),
            result.data(), &length
        );
        return result;
    }

    std::array<uint8_t, kSha512Size> Pbkdf2::Sha512(const bytes::const_span password, const bytes::const_span salt, const int iterations) {
        auto result = std::array<uint8_t, kSha512Size>();
        PKCS5_PBKDF2_HMAC(
            reinterpret_cast<const char*>(password.data()), static_cast<int>(password.size()),
            reinterpret_cast<const unsigned char*>(salt.data()), static_cast<int>(salt.size()),
            iterations,
            EVP_sha512(),
            result.size(), result.data()
        );
        return result;
    }

    bytes::binary AesCbc::Encrypt(const bytes::const_span data, const std::array<uint8_t, 32>& key, std::array<uint8_t, 16> iv) {
        bytes::binary result(data.size());
        auto aes = AES_KEY();
        AES_set_encrypt_key(key.data(), static_cast<int>(key.size()) * CHAR_BIT, &aes);
        AES_cbc_encrypt(
            reinterpret_cast<const unsigned char*>(data.data()),
            result.data(),
            data.size(),
            &aes,
            iv.data(),
            AES_ENCRYPT
        );
        return result;
    }

    bytes::binary AesCbc::Decrypt(const bytes::const_span data, const std::array<uint8_t, 32>& key, std::array<uint8_t, 16> iv) {
        bytes::binary result(data.size());
        auto aes = AES_KEY();
        AES_set_decrypt_key(key.data(), static_cast<int>(key.size()) * CHAR_BIT, &aes);
        AES_cbc_encrypt(
            reinterpret_cast<const unsigned char*>(data.data()),
            result.data(),
            data.size(),
            &aes,
            iv.data(),
            AES_DECRYPT
        );
        return result;
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
        std::memcpy(aesKey, sha256a.data(), 8);
        std::memcpy(aesKey + 8, sha256b.data() + 8, 16);
        std::memcpy(aesKey + 8 + 16, sha256a.data() + 24, 8);
        std::memcpy(aesIv, sha256b.data(), 4);
        std::memcpy(aesIv + 4, sha256a.data() + 8, 8);
        std::memcpy(aesIv + 4 + 8, sha256b.data() + 24, 4);
        return result;
    }

    void Aes::ProcessCtr(const bytes::memory_span from, void* to, KeyIv& keyIv) {
        auto aes = AES_KEY();
        AES_set_encrypt_key(keyIv.key.data(), keyIv.key.size() * CHAR_BIT, &aes);
        uint8_t eCountBuf[16] = {};
        uint32_t offsetInBlock = 0;
        AES_ctr128_encrypt(
            static_cast<const unsigned char*>(from.data),
            static_cast<unsigned char*>(to),
            from.size,
            &aes,
            keyIv.iv.data(),
            eCountBuf,
            &offsetInBlock
        );
    }
} // openssl