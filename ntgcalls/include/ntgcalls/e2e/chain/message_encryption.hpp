//
// Created by Laky-64 on 17/06/26.
//

#pragma once
#include <array>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/encryption.hpp>

namespace telegram::e2e::chain {
    class MessageEncryption {
        static constexpr std::string_view kEncryptData = "tde2e_encrypt_data";
        static constexpr std::string_view kEncryptHeader = "tde2e_encrypt_header";

        static bytes::binary encryptDataWithPrefix(
            bytes::const_span data,
            bytes::const_span secret,
            bytes::const_span extra,
            std::array<uint8_t, 32>* largeMsgId
        );

        static void appendInt32(bytes::binary& buffer, int32_t value);

        static bool constTimeEqual(const uint8_t* a, const uint8_t* b, size_t size);

        static std::array<uint8_t, 32> sliceKey(const std::array<uint8_t, 64>& hash);

        static std::array<uint8_t, 16> sliceIv(const std::array<uint8_t, 64>& hash);

    public:
        static bytes::binary encryptData(
            bytes::const_span data,
            bytes::const_span secret,
            bytes::const_span extra = {},
            std::array<uint8_t, 32>* largeMsgId = nullptr
        );

        static std::optional<bytes::binary> decryptData(
            bytes::const_span encrypted,
            bytes::const_span secret,
            bytes::const_span extra = {},
            std::array<uint8_t, 32>* largeMsgId = nullptr
        );

        static std::optional<bytes::binary> encryptHeader(
            bytes::const_span decryptedHeader,
            bytes::const_span encryptedMessage,
            bytes::const_span secret
        );

        static std::optional<bytes::binary> decryptHeader(
            bytes::const_span encryptedHeader,
            bytes::const_span encryptedMessage,
            bytes::const_span secret
        );
    };
} // telegram::e2e::chain
