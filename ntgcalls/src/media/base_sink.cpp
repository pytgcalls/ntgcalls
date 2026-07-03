//
// Created by Lauren on 27/09/24.
//

#include <ntgcalls/media/base_sink.hpp>

namespace ntgcalls::media {
    BaseSink::~BaseSink() {
        clear();
    }

    uint64_t BaseSink::time() {
        return std::chrono::duration_cast<std::chrono::seconds>(nano_time()).count();
    }

    std::chrono::nanoseconds BaseSink::nano_time() {
        return frames_ * frame_time();
    }

    void BaseSink::clear() {
        frames_ = 0;
    }
} // ntgcalls::media
