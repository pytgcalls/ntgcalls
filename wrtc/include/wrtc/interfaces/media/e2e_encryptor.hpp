//
// Created by Laky-64 on 19/06/26.
//

#pragma once
#include <wrtc/utils/binary.hpp>

namespace wrtc {
    class E2EEncryptor {
    public:
        virtual ~E2EEncryptor() = default;

        virtual bytes::binary encrypt(const bytes::binary& data, size_t unencryptedPrefix) = 0;

        virtual bytes::binary decrypt(int64_t userId, const bytes::binary& data) = 0;
    };
} // wrtc
