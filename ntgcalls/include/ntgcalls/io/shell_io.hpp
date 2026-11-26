//
// Created by Lauren on 26/11/26.
//

#pragma once

#ifdef BOOST_ENABLED
#include <ntgcalls/io/shell_stream.hpp>
#include <ntgcalls/io/stream_io.hpp>

namespace ntgcalls::io {
    template<typename Format>
    using ShellIO = StreamIO<Format, ShellStream>;
} // ntgcalls::io
#endif
