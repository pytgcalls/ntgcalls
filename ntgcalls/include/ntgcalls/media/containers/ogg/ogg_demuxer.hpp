//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <deque>
#include <ntgcalls/media/containers/demuxer.hpp>

namespace ntgcalls::media::containers {

    class OggDemuxer final: public Demuxer {
        static constexpr size_t kPageHeaderSize = 27;
        static constexpr size_t kMaxPacketSize = 65536;

        bytes::binary buffer_;
        bytes::binary partial_;
        std::deque<std::pair<bytes::unique_binary, size_t>> packets_;
        uint8_t channels_ = 0;
        bool discarding_ = false;

        bool parse_page();

        void handle_packet(const uint8_t* data, size_t size);

        [[nodiscard]] size_t page_size() const;

    public:
        void feed(bytes::unique_binary data, size_t size) override;

        [[nodiscard]] int64_t missing() const override;

        std::optional<DemuxedFrame> next() override;

        [[nodiscard]] uint8_t channels() const;
    };

} // ntgcalls::media::containers
