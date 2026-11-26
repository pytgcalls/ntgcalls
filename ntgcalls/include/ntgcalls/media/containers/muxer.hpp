//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <ntgcalls/media/containers/encoded_packet.hpp>
#include <ntgcalls/media/containers/muxed_chunk.hpp>

namespace ntgcalls::media::containers {

    class Muxer {
    public:
        virtual ~Muxer() = default;

        virtual MuxedChunk add(EncodedPacket packet) = 0;

        virtual MuxedChunk finish() = 0;
    };

} // ntgcalls::media::containers
