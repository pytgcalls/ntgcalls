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

        void feed(bytes::unique_binary data, size_t size) override;

        [[nodiscard]] int64_t missing() const override;

        std::optional<DemuxedFrame> next() override;
    };

} // ntgcalls::media::containers
