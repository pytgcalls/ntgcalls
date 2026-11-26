//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <optional>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::media::containers {

    class ByteSource {
    public:
        virtual ~ByteSource() = default;

        virtual bytes::unique_binary read(int64_t size) = 0;

        virtual size_t read_some(uint8_t* data, size_t size) = 0;

        virtual std::optional<int64_t> seek(int64_t offset, int whence) = 0;
    };

} // ntgcalls::media::containers
