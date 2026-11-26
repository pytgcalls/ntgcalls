//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <chrono>
#include <functional>
#include <vector>
#include <ntgcalls/media/base_sink.hpp>
#include <ntgcalls/media/containers/encoded_packet.hpp>
#include <ntgcalls/media/containers/media_kind.hpp>
#include <ntgcalls/media/containers/track_info.hpp>
#include <wrtc/models/frame.hpp>

namespace ntgcalls::media::mixers {

    class Mixer {
    public:
        using CodecFilter = std::function<bool(wrtc::models::Codec)>;
        using PacketCallback = std::function<void(const containers::TrackInfo&, containers::EncodedPacket)>;

    protected:
        BaseSink* sink_;
        CodecFilter supports_;
        PacketCallback packet_callback_;

    public:
        Mixer(BaseSink* sink, CodecFilter supports, PacketCallback packet_callback);

        virtual ~Mixer() = default;

        virtual void send_frames(std::vector<wrtc::models::Frame> frames) = 0;

        virtual void remove_source(int64_t ssrc);

        virtual void tick();

        [[nodiscard]] virtual std::chrono::nanoseconds tick_interval() const = 0;

        virtual void flush();
    };

} // ntgcalls::media::mixers
