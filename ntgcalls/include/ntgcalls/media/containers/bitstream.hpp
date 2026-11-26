//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <optional>
#include <utility>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::media::containers {

    class Bitstream {
        static std::optional<std::pair<size_t, size_t>> next_nal(const uint8_t* data, size_t size, size_t offset);

        static std::optional<std::pair<uint8_t, size_t>> next_obu(const uint8_t* data, size_t size, size_t offset);

    public:
        static bool has_nal_type(const uint8_t* data, size_t size, uint8_t type);

        static bool has_obu_type(const uint8_t* data, size_t size, uint8_t type);

        static bytes::binary h264_parameter_sets(const uint8_t* data, size_t size);

        static bytes::binary av1_sequence_header(const uint8_t* data, size_t size);
    };

} // ntgcalls::media::containers
