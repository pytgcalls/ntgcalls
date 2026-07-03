//
// Created by Lauren on 08/04/25.
//

#pragma once

#include <chrono>

namespace wrtc::utils {

    class SyncHelper {
        std::chrono::nanoseconds frame_time_;
        std::chrono::steady_clock::time_point next_frame_time_;
    public:
        explicit SyncHelper(std::chrono::nanoseconds frame_time);

        void synchronize_time(std::chrono::steady_clock::time_point time = std::chrono::steady_clock::time_point{});

        void wait_next_frame();
    };

} // wrtc::utils
