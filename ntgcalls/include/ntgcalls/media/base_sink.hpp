//
// Created by Laky64 on 27/09/24.
//

#pragma once
#include <chrono>
#include <api/media_stream_interface.h>
#include <api/scoped_refptr.h>

namespace ntgcalls {

    class BaseSink {
    protected:
        uint64_t frames = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;

        void clear();

    public:
        virtual ~BaseSink();

        virtual rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> createTrack() = 0;

        uint64_t time();

        std::chrono::nanoseconds nanoTime();

        virtual int64_t frameSize() = 0;

        virtual std::chrono::nanoseconds frameTime() = 0;
    };

} // ntgcalls
