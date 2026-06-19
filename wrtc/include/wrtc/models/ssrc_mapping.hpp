//
// Created by laky64 on 07/06/26.
//

#pragma once
#include <cstdint>

namespace wrtc {
    class SsrcMapping {
    public:
        int64_t userID;
        int32_t ssrc;

        SsrcMapping(int64_t userID, int32_t ssrc);
    };
} // wrtc
