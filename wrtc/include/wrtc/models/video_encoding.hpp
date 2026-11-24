//
// Created by Lauren on 20/11/26.
//

#pragma once
#include <memory>
#include <wrtc/utils/encoded/encoded_queue.hpp>

namespace wrtc::models {
    struct VideoEncoding {
        std::shared_ptr<utils::encoded::EncodedQueue> passthrough;

        bool operator==(const VideoEncoding& rhs) const = default;
    };
} // wrtc::models
