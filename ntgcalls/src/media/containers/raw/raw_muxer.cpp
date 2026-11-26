//
// Created by Lauren on 26/11/26.
//

#include <ntgcalls/media/containers/raw/raw_muxer.hpp>

namespace ntgcalls::media::containers {
    RawMuxer::RawMuxer(const TrackInfo&) {}

    bool RawMuxer::supports(const wrtc::models::Codec codec) {
        return codec == wrtc::models::Codec::PCM || codec == wrtc::models::Codec::I420;
    }

    MuxedChunk RawMuxer::add(EncodedPacket packet) {
        return {std::move(packet.data), packet.size};
    }

    MuxedChunk RawMuxer::finish() {
        return {};
    }
} // ntgcalls::media::containers
