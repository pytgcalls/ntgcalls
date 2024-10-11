//
// Created by Laky64 on 27/09/24.
//

#pragma once
#include <chrono>

namespace ntgcalls {

    class BaseSink {
    protected:
        uint64_t frames = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;

        void clear();

    public:
        virtual ~BaseSink();

        uint64_t time();

        std::chrono::nanoseconds nanoTime();

        virtual int64_t frameSize() = 0;

        virtual std::chrono::nanoseconds frameTime() = 0;
    };

} // ntgcalls
