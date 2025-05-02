//
// Created by Laky64 on 08/04/25.
//

#include <thread>
#include <wrtc/utils/sync_helper.hpp>

namespace wrtc {

    SyncHelper::SyncHelper(const std::chrono::nanoseconds frameTime): frameTime(frameTime) {}

    void SyncHelper::synchronizeTime(const std::chrono::steady_clock::time_point time) {
        if (time <= std::chrono::steady_clock::time_point{}) {
            nextFrameTime = std::chrono::steady_clock::now();
        } else {
            nextFrameTime = time;
        }
    }

    void SyncHelper::waitNextFrame() {
        nextFrameTime += frameTime;
        std::this_thread::sleep_until(nextFrameTime);
    }

} // wrtc