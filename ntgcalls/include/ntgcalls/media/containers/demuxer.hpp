//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <ntgcalls/media/containers/demuxed_frame.hpp>

namespace ntgcalls::media::containers {

    class Demuxer {
    public:
        virtual ~Demuxer() = default;

        virtual void feed(bytes::unique_binary data, size_t size) = 0;

        [[nodiscard]] virtual int64_t missing() const = 0;

        virtual std::optional<DemuxedFrame> next() = 0;
    };

} // ntgcalls::media::containers
