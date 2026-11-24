//
// Created by Lauren on 08/10/24.
//

#pragma once

#ifdef BOOST_ENABLED
#include <ntgcalls/io/shell_stream.hpp>
#include <ntgcalls/io/stream_io.hpp>

namespace ntgcalls::io {
    template<typename FormatWriter>
    using ShellWriter = StreamIO<FormatWriter, ShellStream>;
} // ntgcalls::io
#endif
