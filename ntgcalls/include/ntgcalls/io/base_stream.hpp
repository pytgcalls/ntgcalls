//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <optional>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::io {
    class BaseStream {
    public:
        enum class Mode {
            Read,
            Write
        };

        virtual ~BaseStream() = default;

        virtual bytes::unique_binary read(int64_t size) = 0;

        virtual size_t read_some(uint8_t* data, size_t size) = 0;

        virtual std::optional<int64_t> seek(int64_t offset, int whence);

        virtual void write(const bytes::unique_binary& data, size_t size) = 0;

        virtual void close() = 0;

        virtual void interrupt() {}
    };
} // ntgcalls::io
