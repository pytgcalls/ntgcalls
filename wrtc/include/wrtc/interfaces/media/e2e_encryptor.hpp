//
// Created by Lauren on 19/06/26.
//

#pragma once
#include <wrtc/utils/binary.hpp>

namespace wrtc::interfaces::media {
    class E2EEncryptor {
    public:
        virtual ~E2EEncryptor() = default;

        virtual bytes::binary encrypt(const bytes::binary& data, size_t unencrypted_prefix) = 0;

        virtual bytes::binary decrypt(int64_t user_id, const bytes::binary& data) = 0;
    };
} // wrtc::interfaces::media
