//
// Created by Lauren on 20/11/26.
//

#pragma once

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

        virtual void write(const bytes::unique_binary& data, size_t size) = 0;

        virtual void close() = 0;
    };
} // ntgcalls::io
