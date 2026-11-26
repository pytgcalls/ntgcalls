//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <chrono>
#include <optional>
#include <wrtc/models/frame_data.hpp>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::media::containers {
    struct DemuxedFrame {
        bytes::unique_binary data;
        size_t size = 0;
        std::optional<int64_t> timestamp;
        std::optional<std::chrono::nanoseconds> duration;
        wrtc::models::FrameData frame_data;
    };
} // ntgcalls::media::containers
