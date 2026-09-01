//
// Created by Lauren on 07/06/26.
//

#pragma once
#include <cstdint>

namespace wrtc::models {
    class SsrcMapping {
    public:
        int64_t user_id;
        int32_t ssrc;

        SsrcMapping(int64_t user_id, int32_t ssrc);
    };
} // wrtc::models
