//
// Created by Lauren on 14/11/26.
//

#pragma once
#include <cstdint>
#include <optional>

namespace wrtc::models {
    struct AudioEncoding {
        std::optional<uint32_t> bitrate;
        uint8_t channel_count = 1;

        bool operator==(const AudioEncoding& rhs) const = default;
    };
} // wrtc::models
