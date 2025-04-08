//
// Created by Laky64 on 08/04/25.
//

#pragma once

#include <chrono>
#include <ntgcalls/media/base_sink.hpp>

namespace ntgcalls {
    using std::chrono::operator ""ns;
    using std::chrono::operator ""ms;

    class SyncHelper {
        std::chrono::nanoseconds nextFrameTime = 0ns;
        std::chrono::nanoseconds frameTime;

    protected:
        void waitNextFrame();

    public:
        explicit SyncHelper(BaseSink *sink);

        void synchronizeTime(std::chrono::milliseconds time = 0ms);
    };

} // ntgcalls
