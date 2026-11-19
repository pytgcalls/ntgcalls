//
// Created by Lauren on 04/08/23.
//

#pragma once

#include <ntgcalls/io/file_stream.hpp>
#include <ntgcalls/io/stream_io.hpp>

namespace ntgcalls::io {
    template<typename FormatReader>
    using FileReader = StreamIO<FormatReader, FileStream>;
} // ntgcalls::io
