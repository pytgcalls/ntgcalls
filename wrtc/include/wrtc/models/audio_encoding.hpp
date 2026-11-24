//
// Created by Lauren on 14/11/26.
//

#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <wrtc/utils/encoded/encoded_queue.hpp>

namespace wrtc::models {
    struct AudioEncoding {
        std::optional<uint32_t> bitrate;
        uint8_t channel_count = 1;
        std::shared_ptr<utils::encoded::EncodedQueue> passthrough;

        bool operator==(const AudioEncoding& rhs) const = default;
    };
} // wrtc::models
