//
// Created by Laky64 on 08/03/2024.
//

#pragma once
#include "wrtc/utils/binary.hpp"

namespace ntgcalls {
    static constexpr auto kSize = 256;
    using Key = std::shared_ptr<std::array<uint8_t, kSize>>;
    using RawKey = std::array<bytes::byte, kSize>;

    class AuthKey {

    public:
        static bytes::vector CreateAuthKey(bytes::const_span firstBytes, bytes::const_span random, bytes::const_span primeBytes);

        static void FillData(RawKey authKey, bytes::const_span computedAuthKey);

        static uint64_t Fingerprint(bytes::const_span authKey);
    };
} // ntgcalls
