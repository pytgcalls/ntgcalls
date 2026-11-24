//
// Created by Lauren on 30/08/23.
//

#pragma once

#ifdef BOOST_ENABLED
#include <ntgcalls/io/shell_stream.hpp>
#include <ntgcalls/io/stream_io.hpp>

namespace ntgcalls::io {
    template<typename FormatReader>
    using ShellReader = StreamIO<FormatReader, ShellStream>;
} // ntgcalls::io
#endif
