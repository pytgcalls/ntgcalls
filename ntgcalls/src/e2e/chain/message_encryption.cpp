//
// Created by Laky-64 on 17/06/26.
//

#include <ntgcalls/e2e/chain/message_encryption.hpp>
#include <wrtc/utils/encryption.hpp>
#include <wrtc/utils/random.hpp>

namespace telegram::e2e::chain {
    void MessageEncryption::appendInt32(bytes::binary &buffer, const int32_t value) {
        const auto raw = static_cast<uint32_t>(value);
        for (int i = 0; i < 4; ++i) {
            buffer.push_back(static_cast<uint8_t>(raw >> i * 8 & 0xff));
        }
    }

    // ReSharper disable once CppDFAConstantParameter
    bool MessageEncryption::constTimeEqual(const uint8_t *a, const uint8_t *b, const size_t size) {
        uint8_t diff = 0;
        for (size_t i = 0; i < size; ++i) {
            diff |= a[i] ^ b[i];
        }
        return diff == 0;
    }

    std::array<uint8_t, 32> MessageEncryption::sliceKey(const std::array<uint8_t, 64> &hash) {
        std::array<uint8_t, 32> key{};
        std::copy_n(hash.begin(), 32, key.begin());
        return key;
    }

    std::array<uint8_t, 16> MessageEncryption::sliceIv(const std::array<uint8_t, 64> &hash) {
        std::array<uint8_t, 16> iv{};
        std::copy_n(hash.begin() + 32, 16, iv.begin());
        return iv;
    }

    bytes::binary MessageEncryption::encryptDataWithPrefix(
        const bytes::const_span data,
        const bytes::const_span secret,
        const bytes::const_span extra,
        std::array<uint8_t, 32>* largeMsgId
    ) {
        const auto large = openssl::Hmac::Sha512(secret, bytes::view(kEncryptData));
        const auto encryptSecret = bytes::view(large.data(), 32);
        const auto hmacSecret = bytes::view(large.data() + 32, 32);

        bytes::binary tail;
        tail.reserve(data.size() + extra.size() + 4);
        tail.insert(tail.end(), reinterpret_cast<const uint8_t*>(data.data()), reinterpret_cast<const uint8_t*>(data.data()) + data.size());
        tail.insert(tail.end(), reinterpret_cast<const uint8_t*>(extra.data()), reinterpret_cast<const uint8_t*>(extra.data()) + extra.size());
        appendInt32(tail, static_cast<int32_t>(extra.size()));

        const auto largeId = openssl::Hmac::Sha256(hmacSecret, bytes::view(tail));
        // ReSharper disable once CppDFAConstantConditions
        if (largeMsgId) {
            *largeMsgId = largeId;
        }
        const auto cbc = openssl::Hmac::Sha512(encryptSecret, bytes::view(largeId.data(), 16));
        const auto encrypted = openssl::AesCbc::Encrypt(data, sliceKey(cbc), sliceIv(cbc));

        bytes::binary result;
        result.reserve(16 + encrypted.size());
        result.insert(result.end(), largeId.begin(), largeId.begin() + 16);
        result.insert(result.end(), encrypted.begin(), encrypted.end());
        return result;
    }

    bytes::binary MessageEncryption::encryptData(
        const bytes::const_span data,
        const bytes::const_span secret,
        const bytes::const_span extra,
        std::array<uint8_t, 32>* largeMsgId
    ) {
        const auto dataSize = data.size();
        const auto prefixSize = (16 + 15 + dataSize & ~static_cast<size_t>(15)) - dataSize;
        bytes::binary combined(prefixSize + dataSize);
        bytes::RandomFill(bytes::span(reinterpret_cast<std::byte*>(combined.data()), prefixSize));
        combined[0] = static_cast<uint8_t>(prefixSize);
        std::copy_n(reinterpret_cast<const uint8_t*>(data.data()), dataSize, combined.data() + prefixSize);
        return encryptDataWithPrefix(bytes::view(combined), secret, extra, largeMsgId);
    }

    std::optional<bytes::binary> MessageEncryption::decryptData(
        const bytes::const_span encrypted,
        const bytes::const_span secret,
        const bytes::const_span extra,
        std::array<uint8_t, 32>* largeMsgId
    ) {
        if (encrypted.size() < 16 || encrypted.size() % 16 != 0) {
            return std::nullopt;
        }
        const auto large = openssl::Hmac::Sha512(secret, bytes::view(kEncryptData));
        const auto encryptSecret = bytes::view(large.data(), 32);
        const auto hmacSecret = bytes::view(large.data() + 32, 32);

        const auto msgId = encrypted.subspan(0, 16);
        const auto payload = encrypted.subspan(16);

        const auto cbc = openssl::Hmac::Sha512(encryptSecret, msgId);
        auto decrypted = openssl::AesCbc::Decrypt(payload, sliceKey(cbc), sliceIv(cbc));

        bytes::binary buffer = decrypted;
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(extra.data()), reinterpret_cast<const uint8_t*>(extra.data()) + extra.size());
        appendInt32(buffer, static_cast<int32_t>(extra.size()));

        const auto expected = openssl::Hmac::Sha256(hmacSecret, bytes::view(buffer));
        if (!constTimeEqual(expected.data(), reinterpret_cast<const uint8_t*>(msgId.data()), 16)) {
            return std::nullopt;
        }
        // ReSharper disable once CppDFAConstantConditions
        if (largeMsgId) {
            *largeMsgId = expected;
        }
        const auto prefixSize = static_cast<size_t>(decrypted[0]);
        if (prefixSize > decrypted.size() || prefixSize < 16) {
            return std::nullopt;
        }
        return bytes::binary(decrypted.begin() + static_cast<std::ptrdiff_t>(prefixSize), decrypted.end());
    }

    std::optional<bytes::binary> MessageEncryption::encryptHeader(
        const bytes::const_span decryptedHeader,
        const bytes::const_span encryptedMessage,
        const bytes::const_span secret
    ) {
        if (encryptedMessage.size() < 16 || decryptedHeader.size() != 32) {
            return std::nullopt;
        }
        const auto largeKey = openssl::Hmac::Sha512(secret, bytes::view(kEncryptHeader));
        const auto encryptionKey = bytes::view(largeKey.data(), 32);
        const auto cbc = openssl::Hmac::Sha512(encryptionKey, encryptedMessage.subspan(0, 16));
        return openssl::AesCbc::Encrypt(decryptedHeader, sliceKey(cbc), sliceIv(cbc));
    }

    std::optional<bytes::binary> MessageEncryption::decryptHeader(
        const bytes::const_span encryptedHeader,
        const bytes::const_span encryptedMessage,
        const bytes::const_span secret
    ) {
        if (encryptedMessage.size() < 16 || encryptedHeader.size() != 32) {
            return std::nullopt;
        }
        const auto largeKey = openssl::Hmac::Sha512(secret, bytes::view(kEncryptHeader));
        const auto encryptionKey = bytes::view(largeKey.data(), 32);
        const auto cbc = openssl::Hmac::Sha512(encryptionKey, encryptedMessage.subspan(0, 16));
        return openssl::AesCbc::Decrypt(encryptedHeader, sliceKey(cbc), sliceIv(cbc));
    }
} // telegram::e2e::chain