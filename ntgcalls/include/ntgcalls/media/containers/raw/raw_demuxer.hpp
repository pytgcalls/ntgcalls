//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <ntgcalls/media/base_sink.hpp>
#include <ntgcalls/media/containers/demuxer.hpp>

namespace ntgcalls::media::containers {

    class RawDemuxer final: public Demuxer {
        size_t frame_size_;
        std::chrono::nanoseconds frame_time_;
        size_t batch_size_;
        bytes::unique_binary chunk_;
        size_t chunk_size_ = 0;
        size_t offset_ = 0;

    public:
        explicit RawDemuxer(BaseSink* sink);

        std::optional<DemuxedFrame> next(ByteSource& source) override;
    };

} // ntgcalls::media::containers
