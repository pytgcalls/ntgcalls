//
// Created by Lauren on 07/10/24.
//

#pragma once

#include <ntgcalls/io/file_stream.hpp>
#include <ntgcalls/io/stream_io.hpp>

namespace ntgcalls::io {
    template<typename FormatWriter>
    using FileWriter = StreamIO<FormatWriter, FileStream>;
} // ntgcalls::io
