//
// Created by Lauren on 19/11/26.
//

#pragma once

#include <ntgcalls/io/threaded_reader.hpp>

namespace ntgcalls::io {

    class RawReader: public ThreadedReader {
    protected:
        std::vector<Chunk> read_frames(std::chrono::nanoseconds duration) override;

    public:
        explicit RawReader(media::BaseSink* sink);
    };

} // ntgcalls::io
