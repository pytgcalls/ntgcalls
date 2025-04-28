//
// Created by Laky64 on 08/04/25.
//

#pragma once

#include <chrono>
#include <ntgcalls/media/base_sink.hpp>

namespace ntgcalls {

    class SyncHelper {
        std::chrono::steady_clock::time_point nextFrameTime;
        std::chrono::nanoseconds frameTime;

    protected:
        void waitNextFrame();

    public:
        explicit SyncHelper(BaseSink *sink);

        void synchronizeTime(std::chrono::steady_clock::time_point time = std::chrono::steady_clock::time_point{});
    };

} // ntgcalls
