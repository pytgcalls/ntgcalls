//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <ntgcalls/media/containers/byte_source.hpp>
#include <ntgcalls/media/containers/demuxed_frame.hpp>

namespace ntgcalls::media::containers {

    class Demuxer {
    public:
        virtual ~Demuxer() = default;

        virtual std::optional<DemuxedFrame> next(ByteSource& source) = 0;
    };

} // ntgcalls::media::containers
