//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <ntgcalls/io/file_stream.hpp>
#include <ntgcalls/io/stream_io.hpp>

namespace ntgcalls::io {
    template<typename Format>
    using FileIO = StreamIO<Format, FileStream>;
} // ntgcalls::io
