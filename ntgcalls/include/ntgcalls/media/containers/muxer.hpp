//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <ntgcalls/media/containers/encoded_packet.hpp>

namespace ntgcalls::media::containers {

    class Muxer {
    public:
        virtual ~Muxer() = default;

        virtual bytes::binary add(const EncodedPacket& packet) = 0;

        virtual bytes::binary finish() = 0;
    };

} // ntgcalls::media::containers
