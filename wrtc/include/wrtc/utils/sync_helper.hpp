//
// Created by Laky64 on 08/04/25.
//

#pragma once

#include <chrono>

namespace wrtc {

    class SyncHelper {
        std::chrono::nanoseconds frameTime;
        std::chrono::steady_clock::time_point nextFrameTime;
    public:
        explicit SyncHelper(std::chrono::nanoseconds frameTime);

        void synchronizeTime(std::chrono::steady_clock::time_point time = std::chrono::steady_clock::time_point{});

        void waitNextFrame();
    };

} // wrtc
