//
// Created by Lauren on 06/10/26.
//

#pragma once

#include <cstdint>
#include <vector>

namespace wrtc::models {
    struct ParticipantsRequest {
        std::vector<uint32_t> ssrcs;
    };
} // wrtc::models
