//
// Created by Lauren on 17/06/26.
//

#pragma once
#include <array>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/encryption.hpp>

namespace ntgcalls::e2e::chain {
    class MessageEncryption {
        static constexpr std::string_view kEncryptData = "tde2e_encrypt_data";
        static constexpr std::string_view kEncryptHeader = "tde2e_encrypt_header";

        static bytes::binary encrypt_data_with_prefix(
            bytes::const_span data,
            bytes::const_span secret,
            bytes::const_span extra,
            std::array<uint8_t, 32>* large_msg_id
        );

        static void append_int32(bytes::binary& buffer, int32_t value);

        static bool const_time_equal(const uint8_t* a, const uint8_t* b, size_t size);

        static std::array<uint8_t, 32> slice_key(const std::array<uint8_t, 64>& hash);

        static std::array<uint8_t, 16> slice_iv(const std::array<uint8_t, 64>& hash);

    public:
        static bytes::binary encrypt_data(
            bytes::const_span data,
            bytes::const_span secret,
            bytes::const_span extra = {},
            std::array<uint8_t, 32>* large_msg_id = nullptr
        );

        static std::optional<bytes::binary> decrypt_data(
            bytes::const_span encrypted,
            bytes::const_span secret,
            bytes::const_span extra = {},
            std::array<uint8_t, 32>* large_msg_id = nullptr
        );

        static std::optional<bytes::binary> encrypt_header(
            bytes::const_span decrypted_header,
            bytes::const_span encrypted_message,
            bytes::const_span secret
        );

        static std::optional<bytes::binary> decrypt_header(
            bytes::const_span encrypted_header,
            bytes::const_span encrypted_message,
            bytes::const_span secret
        );
    };
} // ntgcalls::e2e::chain
