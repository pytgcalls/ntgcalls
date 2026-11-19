//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <deque>
#include <optional>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::media::containers {

    class OggDemuxer {
        static constexpr size_t kPageHeaderSize = 27;

        bytes::binary buffer_;
        bytes::binary partial_;
        std::deque<std::pair<bytes::unique_binary, size_t>> packets_;
        uint8_t channels_ = 0;

        bool parse_page();

        void handle_packet(const uint8_t* data, size_t size);

        [[nodiscard]] size_t page_size() const;

    public:
        void feed(const uint8_t* data, size_t size);

        [[nodiscard]] int64_t missing() const;

        std::optional<std::pair<bytes::unique_binary, size_t>> next();

        [[nodiscard]] uint8_t channels() const;
    };

} // ntgcalls::media::containers
