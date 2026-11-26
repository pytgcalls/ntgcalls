//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <array>
#include <vector>
#include <ntgcalls/media/containers/media_kind.hpp>
#include <ntgcalls/media/containers/muxer.hpp>
#include <ntgcalls/media/containers/track_info.hpp>

namespace ntgcalls::media::containers {

    class OggMuxer final: public Muxer {
        static constexpr int64_t kSampleRate = 48000;
        static constexpr size_t kMaxPagePackets = 50;
        static constexpr size_t kMaxPageSegments = 255;

        uint8_t channels_;
        uint32_t serial_;
        uint32_t sequence_ = 0;
        uint64_t granule_ = 0;
        bool headers_written_ = false;
        std::vector<bytes::binary> packets_;
        size_t segments_ = 0;

        bytes::binary page(const std::vector<bytes::binary>& packets, uint8_t flags);

        void write_headers(std::vector<bytes::binary>& pages);

        static size_t segments(size_t size);

        static std::array<uint32_t, 256> crc_table();

        static uint32_t crc(const bytes::binary& page);

        static bytes::binary join(const std::vector<bytes::binary>& pages);

    public:
        explicit OggMuxer(const TrackInfo& track);

        bytes::binary add(const EncodedPacket& packet) override;

        bytes::binary finish() override;

        static constexpr bool supports(const MediaKind kind) {
            return kind == MediaKind::Audio;
        }

        static bool supports(wrtc::models::Codec codec);

        [[nodiscard]] uint64_t granule() const;
    };

} // ntgcalls::media::containers
