//
// Created by Lauren on 08/04/25.
//

#include <thread>
#include <wrtc/utils/sync_helper.hpp>

namespace wrtc::utils {

    SyncHelper::SyncHelper(const std::chrono::nanoseconds frame_time): frame_time_(frame_time) {}

    void SyncHelper::synchronize_time(const std::chrono::steady_clock::time_point time) {
        const std::lock_guard lock(mutex_);
        if (time <= std::chrono::steady_clock::time_point{}) {
            next_frame_time_ = std::chrono::steady_clock::now();
        } else {
            next_frame_time_ = time;
        }
    }

    void SyncHelper::wait_next_frame() {
        wait_next_frame(frame_time_);
    }

    void SyncHelper::wait_next_frame(const std::chrono::nanoseconds duration) {
        std::chrono::steady_clock::time_point deadline;
        {
            const std::lock_guard lock(mutex_);
            next_frame_time_ += duration;
            deadline = next_frame_time_;
        }
        std::this_thread::sleep_until(deadline);
    }

} // wrtc::utils
