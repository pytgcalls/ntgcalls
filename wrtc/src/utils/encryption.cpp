//
// Created by Lauren on 09/03/24.
//

#include <wrtc/utils/encryption.hpp>

#include <climits>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

namespace openssl {
    bytes::binary Sha256::digest(const bytes::const_span data) {
        auto bytes = bytes::binary(SHA256_DIGEST_LENGTH);
        SHA256(data.data(), data.size(), bytes.data());
        return bytes;
    }

    bytes::array<kSha512Size> Sha512::digest(const bytes::const_span data) {
        auto result = bytes::array<kSha512Size>();
        SHA512(data.data(), data.size(), result.data());
        return result;
    }

    bytes::array<kSha256Size> Hmac::sha256(const bytes::const_span key, const bytes::const_span data) {
        auto result = bytes::array<kSha256Size>();
        unsigned int length = 0;
        HMAC(
            EVP_sha256(),
            key.data(), static_cast<int>(key.size()),
            data.data(), data.size(),
            result.data(), &length
        );
        return result;
    }

    bytes::array<kSha512Size> Hmac::sha512(const bytes::const_span key, const bytes::const_span data) {
        auto result = bytes::array<kSha512Size>();
        unsigned int length = 0;
        HMAC(
            EVP_sha512(),
            key.data(), static_cast<int>(key.size()),
            data.data(), data.size(),
            result.data(), &length
        );
        return result;
    }

    bytes::array<kSha512Size> Pbkdf2::sha512(const bytes::const_span password, const bytes::const_span salt, const int iterations) {
        auto result = bytes::array<kSha512Size>();
        PKCS5_PBKDF2_HMAC(
            reinterpret_cast<const char*>(password.data()), static_cast<int>(password.size()),
            salt.data(), static_cast<int>(salt.size()),
            iterations,
            EVP_sha512(),
            result.size(), result.data()
        );
        return result;
    }

    bytes::binary AesCbc::encrypt(const bytes::const_span data, const bytes::array<32>& key, bytes::array<16> iv) {
        bytes::binary result(data.size());
        auto aes = AES_KEY();
        AES_set_encrypt_key(key.data(), static_cast<int>(key.size()) * CHAR_BIT, &aes);
        AES_cbc_encrypt(
            data.data(),
            result.data(),
            data.size(),
            &aes,
            iv.data(),
            AES_ENCRYPT
        );
        return result;
    }

    bytes::binary AesCbc::decrypt(const bytes::const_span data, const bytes::array<32>& key, bytes::array<16> iv) {
        bytes::binary result(data.size());
        auto aes = AES_KEY();
        AES_set_decrypt_key(key.data(), static_cast<int>(key.size()) * CHAR_BIT, &aes);
        AES_cbc_encrypt(
            data.data(),
            result.data(),
            data.size(),
            &aes,
            iv.data(),
            AES_DECRYPT
        );
        return result;
    }

    bytes::array<kSha256Size> Sha256::concat(const bytes::memory_span& first, const bytes::memory_span& second) {
        auto result = bytes::array<kSha256Size>();
        auto context = SHA256_CTX();
        SHA256_Init(&context);
        SHA256_Update(&context, first.data, first.size);
        SHA256_Update(&context, second.data, second.size);
        SHA256_Final(result.data(), &context);
        return result;
    }

    bytes::binary Sha1::digest(const bytes::const_span data) {
        auto bytes = bytes::binary(SHA_DIGEST_LENGTH);
        SHA1(data.data(), data.size(), bytes.data());
        return bytes;
    }

    Aes::KeyIv Aes::prepare_key_iv(const bytes::byte* key, const bytes::byte* msg_key, const int x) {
        auto result = KeyIv();
        const auto sha256a = Sha256::concat(
            bytes::memory_span(msg_key, 16),
            bytes::memory_span(key + x, 36)
        );
        const auto sha256b = Sha256::concat(
            bytes::memory_span(key + 40 + x, 36),
            bytes::memory_span(msg_key, 16)
        );
        const auto aes_key = result.key.data();
        const auto aes_iv = result.iv.data();
        std::memcpy(aes_key, sha256a.data(), 8);
        std::memcpy(aes_key + 8, sha256b.data() + 8, 16);
        std::memcpy(aes_key + 8 + 16, sha256a.data() + 24, 8);
        std::memcpy(aes_iv, sha256b.data(), 4);
        std::memcpy(aes_iv + 4, sha256a.data() + 8, 8);
        std::memcpy(aes_iv + 4 + 8, sha256b.data() + 24, 4);
        return result;
    }

    void Aes::process_ctr(const bytes::memory_span from, void* to, KeyIv& key_iv) {
        auto aes = AES_KEY();
        AES_set_encrypt_key(key_iv.key.data(), key_iv.key.size() * CHAR_BIT, &aes);
        bytes::byte e_count_buf[16] = {};
        uint32_t offset_in_block = 0;
        AES_ctr128_encrypt(
            static_cast<const unsigned char*>(from.data),
            static_cast<unsigned char*>(to),
            from.size,
            &aes,
            key_iv.iv.data(),
            e_count_buf,
            &offset_in_block
        );
    }
} // openssl