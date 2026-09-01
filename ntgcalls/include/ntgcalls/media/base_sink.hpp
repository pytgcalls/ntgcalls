//
// Created by Lauren on 27/09/24.
//

#pragma once
#include <chrono>

namespace ntgcalls::media {

    class BaseSink {
    protected:
        uint64_t frames_ = 0;

        void clear();

    public:
        virtual ~BaseSink();

        uint64_t time();

        std::chrono::nanoseconds nano_time();

        virtual int64_t frame_size() = 0;

        virtual uint8_t frame_rate() = 0;

        virtual std::chrono::nanoseconds frame_time() = 0;
    };

} // ntgcalls::media
