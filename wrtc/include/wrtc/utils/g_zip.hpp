//
// Created by Lauren on 15/03/24.
//

#pragma once
#include <optional>
#include <wrtc/utils/binary.hpp>

namespace bytes {

    class GZip {
        static constexpr uint32_t kChunkSize = 16384;

    public:
        static bool is_gzip(const binary& data);

        static binary zip(const binary& data);

        static std::optional<binary> unzip(const binary& data, size_t size_limit);
    };

} // bytes
