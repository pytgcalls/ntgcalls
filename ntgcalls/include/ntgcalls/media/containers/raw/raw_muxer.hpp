//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <ntgcalls/media/containers/media_kind.hpp>
#include <ntgcalls/media/containers/muxer.hpp>
#include <ntgcalls/media/containers/track_info.hpp>

namespace ntgcalls::media::containers {

    class RawMuxer final: public Muxer {
    public:
        explicit RawMuxer(const TrackInfo& track);

        MuxedChunk add(EncodedPacket packet) override;

        MuxedChunk finish() override;

        static constexpr bool supports(MediaKind) {
            return true;
        }

        static bool supports(wrtc::models::Codec codec);
    };

} // ntgcalls::media::containers
