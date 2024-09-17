//
// Created by Laky64 on 08/03/2024.
//

#pragma once
#include <wrtc/utils/binary.hpp>

namespace signaling {
    struct EncryptionKey {
        static constexpr int kSize = 256;

        std::shared_ptr<const std::array<uint8_t, kSize>> value;
        bool isOutgoing = false;

        EncryptionKey(
            std::shared_ptr<std::array<uint8_t, kSize>> value,
            const bool isOutgoing
        ): value(std::move(value)), isOutgoing(isOutgoing) {}
    };
    using RawKey = std::array<bytes::byte, EncryptionKey::kSize>;

    class AuthKey {
    public:
        static bytes::vector CreateAuthKey(bytes::const_span firstBytes, bytes::const_span random, bytes::const_span primeBytes);

        static void FillData(RawKey &authKey, bytes::const_span computedAuthKey);

        static uint64_t Fingerprint(bytes::const_span authKey);
    };
} // signaling
