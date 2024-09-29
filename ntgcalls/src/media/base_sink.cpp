//
// Created by Laky64 on 27/09/24.
//

#include <ntgcalls/media/base_sink.hpp>

namespace ntgcalls {
    BaseSink::~BaseSink() {
        clear();
    }

    uint64_t BaseSink::time() {
        return std::chrono::duration_cast<std::chrono::seconds>(nanoTime()).count();
    }

    std::chrono::nanoseconds BaseSink::nanoTime() {
        return frames * frameTime();
    }

    void BaseSink::clear() {
        frames = 0;
    }
}
