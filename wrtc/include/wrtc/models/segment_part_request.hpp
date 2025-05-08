//
// Created by Laky64 on 13/04/25.
//

#pragma once

#include <wrtc/models/media_segment.hpp>

namespace wrtc {

    struct SegmentPartRequest {
        static constexpr int32_t DEFAULT_SIZE = 128 * 1024;

        int64_t segmentId;
        int32_t partId;
        int32_t limit;
        int64_t timestamp;
        bool qualityUpdate;
        int32_t channelId;
        MediaSegment::Quality quality;

        SegmentPartRequest(
            const int64_t segmentId,
            const int32_t partId,
            const int64_t limit,
            const int64_t timestamp,
            const bool qualityUpdate,
            const int32_t channelId,
            const MediaSegment::Quality quality
        ) : segmentId(segmentId), partId(partId), limit(limit), timestamp(timestamp),
            qualityUpdate(qualityUpdate), channelId(channelId), quality(quality) {}
    };

} // wrtc
