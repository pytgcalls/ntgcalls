//
// Created by Lauren on 26/11/26.
//

#include <ntgcalls/media/mixers/mixer.hpp>

namespace ntgcalls::media::mixers {
    Mixer::Mixer(BaseSink* sink, CodecFilter supports, PacketCallback packet_callback): sink_(sink), supports_(std::move(supports)), packet_callback_(std::move(packet_callback)) {}

    void Mixer::remove_source(int64_t) {}

    void Mixer::tick() {}

    void Mixer::flush() {}
} // ntgcalls::media::mixers
