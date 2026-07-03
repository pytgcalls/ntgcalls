//
// Created by Lauren on 17/06/26.
//

#include <ntgcalls/e2e/chain/message_encryption.hpp>
#include <wrtc/utils/encryption.hpp>
#include <wrtc/utils/random.hpp>

namespace ntgcalls::e2e::chain {
    void MessageEncryption::append_int32(bytes::binary &buffer, const int32_t value) {
        const auto raw = static_cast<uint32_t>(value);
        for (int i = 0; i < 4; ++i) {
            buffer.push_back(static_cast<uint8_t>(raw >> i * 8 & 0xff));
        }
    }

    // ReSharper disable once CppDFAConstantParameter
    bool MessageEncryption::const_time_equal(const uint8_t *a, const uint8_t *b, const size_t size) {
        uint8_t diff = 0;
        for (size_t i = 0; i < size; ++i) {
            diff |= a[i] ^ b[i];
        }
        return diff == 0;
    }

    std::array<uint8_t, 32> MessageEncryption::slice_key(const std::array<uint8_t, 64> &hash) {
        std::array<uint8_t, 32> key{};
        std::copy_n(hash.begin(), 32, key.begin());
        return key;
    }

    std::array<uint8_t, 16> MessageEncryption::slice_iv(const std::array<uint8_t, 64> &hash) {
        std::array<uint8_t, 16> iv{};
        std::copy_n(hash.begin() + 32, 16, iv.begin());
        return iv;
    }

    bytes::binary MessageEncryption::encrypt_data_with_prefix(
        const bytes::const_span data,
        const bytes::const_span secret,
        const bytes::const_span extra,
        std::array<uint8_t, 32>* large_msg_id
    ) {
        const auto large = openssl::Hmac::sha512(secret, bytes::view(kEncryptData));
        const auto encrypt_secret = bytes::view(large.data(), 32);
        const auto hmac_secret = bytes::view(large.data() + 32, 32);

        bytes::binary tail;
        tail.reserve(data.size() + extra.size() + 4);
        tail.insert(tail.end(), data.data(), data.data() + data.size());
        tail.insert(tail.end(), extra.data(), extra.data() + extra.size());
        append_int32(tail, static_cast<int32_t>(extra.size()));

        const auto large_id = openssl::Hmac::sha256(hmac_secret, bytes::view(tail));
        // ReSharper disable once CppDFAConstantConditions
        if (large_msg_id) {
            *large_msg_id = large_id;
        }
        const auto cbc = openssl::Hmac::sha512(encrypt_secret, bytes::view(large_id.data(), 16));
        const auto encrypted = openssl::AesCbc::encrypt(data, slice_key(cbc), slice_iv(cbc));

        bytes::binary result;
        result.reserve(16 + encrypted.size());
        result.insert(result.end(), large_id.begin(), large_id.begin() + 16);
        result.insert(result.end(), encrypted.begin(), encrypted.end());
        return result;
    }

    bytes::binary MessageEncryption::encrypt_data(
        const bytes::const_span data,
        const bytes::const_span secret,
        const bytes::const_span extra,
        std::array<uint8_t, 32>* large_msg_id
    ) {
        const auto data_size = data.size();
        const auto prefix_size = (16 + 15 + data_size & ~static_cast<size_t>(15)) - data_size;
        bytes::binary combined(prefix_size + data_size);
        bytes::random_fill(bytes::span(combined.data(), prefix_size));
        combined[0] = static_cast<uint8_t>(prefix_size);
        std::copy_n(data.data(), data_size, combined.data() + prefix_size);
        return encrypt_data_with_prefix(bytes::view(combined), secret, extra, large_msg_id);
    }

    std::optional<bytes::binary> MessageEncryption::decrypt_data(
        const bytes::const_span encrypted,
        const bytes::const_span secret,
        const bytes::const_span extra,
        std::array<uint8_t, 32>* large_msg_id
    ) {
        if (encrypted.size() < 16 || encrypted.size() % 16 != 0) {
            return std::nullopt;
        }
        const auto large = openssl::Hmac::sha512(secret, bytes::view(kEncryptData));
        const auto encrypt_secret = bytes::view(large.data(), 32);
        const auto hmac_secret = bytes::view(large.data() + 32, 32);

        const auto msg_id = encrypted.subspan(0, 16);
        const auto payload = encrypted.subspan(16);

        const auto cbc = openssl::Hmac::sha512(encrypt_secret, msg_id);
        auto decrypted = openssl::AesCbc::decrypt(payload, slice_key(cbc), slice_iv(cbc));

        bytes::binary buffer = decrypted;
        buffer.insert(buffer.end(), extra.data(), extra.data() + extra.size());
        append_int32(buffer, static_cast<int32_t>(extra.size()));

        const auto expected = openssl::Hmac::sha256(hmac_secret, bytes::view(buffer));
        if (!const_time_equal(expected.data(), msg_id.data(), 16)) {
            return std::nullopt;
        }
        // ReSharper disable once CppDFAConstantConditions
        if (large_msg_id) {
            *large_msg_id = expected;
        }
        const auto prefix_size = static_cast<size_t>(decrypted[0]);
        if (prefix_size > decrypted.size() || prefix_size < 16) {
            return std::nullopt;
        }
        return bytes::binary(decrypted.begin() + static_cast<std::ptrdiff_t>(prefix_size), decrypted.end());
    }

    std::optional<bytes::binary> MessageEncryption::encrypt_header(
        const bytes::const_span decrypted_header,
        const bytes::const_span encrypted_message,
        const bytes::const_span secret
    ) {
        if (encrypted_message.size() < 16 || decrypted_header.size() != 32) {
            return std::nullopt;
        }
        const auto large_key = openssl::Hmac::sha512(secret, bytes::view(kEncryptData));
        const auto encryption_key = bytes::view(large_key.data(), 32);
        const auto cbc = openssl::Hmac::sha512(encryption_key, encrypted_message.subspan(0, 16));
        return openssl::AesCbc::encrypt(decrypted_header, slice_key(cbc), slice_iv(cbc));
    }

    std::optional<bytes::binary> MessageEncryption::decrypt_header(
        const bytes::const_span encrypted_header,
        const bytes::const_span encrypted_message,
        const bytes::const_span secret
    ) {
        if (encrypted_message.size() < 16 || encrypted_header.size() != 32) {
            return std::nullopt;
        }
        const auto large_key = openssl::Hmac::sha512(secret, bytes::view(kEncryptData));
        const auto encryption_key = bytes::view(large_key.data(), 32);
        const auto cbc = openssl::Hmac::sha512(encryption_key, encrypted_message.subspan(0, 16));
        return openssl::AesCbc::decrypt(encrypted_header, slice_key(cbc), slice_iv(cbc));
    }
} // ntgcalls::e2e::chain