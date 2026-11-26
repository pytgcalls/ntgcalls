//
// Created by Lauren on 27/11/26.
//

#include <ntgcalls/io/base_stream.hpp>

namespace ntgcalls::io {
    std::optional<int64_t> BaseStream::seek(int64_t, int) {
        return std::nullopt;
    }
} // ntgcalls::io
