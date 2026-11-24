//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <array>
#include <vector>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::media::containers {

    class OggMuxer {
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

    public:
        explicit OggMuxer(uint8_t channels);

        std::vector<bytes::binary> add(bytes::binary packet, int samples);

        std::vector<bytes::binary> finish();

        [[nodiscard]] uint64_t granule() const;
    };

} // ntgcalls::media::containers
