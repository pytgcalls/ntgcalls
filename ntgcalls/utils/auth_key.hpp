//
// Created by Laky64 on 08/03/2024.
//

#pragma once
#include "wrtc/utils/binary.hpp"

namespace ntgcalls {
    class AuthKey {
        static constexpr auto kSize = 256;
    public:
        static bytes::binary CreateAuthKey(const bytes::binary& firstBytes, const bytes::binary& random, const bytes::binary& primeBytes);

        static bytes::binary FillData(const bytes::binary& computedAuthKey);

        static uint64_t Fingerprint(const bytes::binary& authKey);
    };
} // ntgcalls
