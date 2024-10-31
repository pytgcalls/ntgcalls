//
// Created by Laky64 on 15/03/2024.
//

#pragma once
#include <optional>

#include <wrtc/utils/binary.hpp>

namespace bytes {

    class GZip {
        static constexpr uint32_t ChunkSize = 16384;
    public:
        static bool isGzip(const binary& data);

        static binary zip(const binary& data);

        static std::optional<binary> unzip(const binary& data, size_t sizeLimit);
    };

} // bytes
