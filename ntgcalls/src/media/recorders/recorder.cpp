//
// Created by Lauren on 26/11/26.
//

#include <ntgcalls/media/recorders/recorder.hpp>

namespace ntgcalls::media::recorders {
    Recorder::Recorder(BaseSink* sink, CodecFilter supports, PacketCallback packet_callback): sink_(sink), supports_(std::move(supports)), packet_callback_(std::move(packet_callback)) {}

    void Recorder::remove_source(int64_t) {}

    void Recorder::tick() {}

    void Recorder::flush() {}
} // ntgcalls::media::recorders
