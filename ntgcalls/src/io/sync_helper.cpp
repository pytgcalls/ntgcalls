//
// Created by Laky64 on 08/04/25.
//

#include <thread>
#include <ntgcalls/io/sync_helper.hpp>
#include <rtc_base/time_utils.h>

namespace ntgcalls {

    SyncHelper::SyncHelper(BaseSink* sink): frameTime(sink->frameTime()) {}

    void SyncHelper::synchronizeTime(const std::chrono::milliseconds time) {
        if (time <= 0ms) {
            nextFrameTime = std::chrono::milliseconds(rtc::TimeMillis());
        } else {
            nextFrameTime = time;
        }
    }

    void SyncHelper::waitNextFrame() {
        nextFrameTime += frameTime;
        std::this_thread::sleep_until(std::chrono::steady_clock::time_point(nextFrameTime));
    }

} // ntgcalls