//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <cstdint>
#include <wrtc/wrtc.hpp>

namespace ntgcalls {
    class BaseStreamer {
        uint64_t sentFrames = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;

    protected:
        ~BaseStreamer();

        virtual std::chrono::nanoseconds frameTime() = 0;

        void clear();

    public:
        uint64_t time();

        std::chrono::nanoseconds nanoTime();

        std::chrono::nanoseconds waitTime();

        virtual wrtc::MediaStreamTrack *createTrack() = 0;

        virtual void sendData(const bytes::shared_binary& sample, int64_t absolute_capture_timestamp_ms);

        virtual int64_t frameSize() = 0;
    };
}
