//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <cstdint>
#include <wrtc/wrtc.hpp>

#include "../exceptions.hpp"

namespace ntgcalls {
    class BaseStreamer {
    private:
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

        virtual void sendData(wrtc::binary &sample);

        virtual uint64_t frameSize() = 0;
    };
}
