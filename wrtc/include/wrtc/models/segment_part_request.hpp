//
// Created by Lauren on 13/04/25.
//

#pragma once

#include <wrtc/models/media_segment.hpp>

namespace wrtc::models {

    struct SegmentPartRequest {
        static constexpr int32_t kDefaultSize = 128 * 1024;

        int64_t segment_id;
        int32_t part_id;
        int32_t limit;
        int64_t timestamp;
        bool quality_update;
        int32_t channel_id;
        MediaSegment::Quality quality;

        SegmentPartRequest(
            const int64_t segment_id,
            const int32_t part_id,
            const int32_t limit,
            const int64_t timestamp,
            const bool quality_update,
            const int32_t channel_id,
            const MediaSegment::Quality quality
        ) : segment_id(segment_id), part_id(part_id), limit(limit), timestamp(timestamp),
            quality_update(quality_update), channel_id(channel_id), quality(quality) {}
    };

} // wrtc::models
