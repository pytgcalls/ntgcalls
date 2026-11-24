//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <functional>
#include <string>
#include <vector>
#include <ntgcalls/io/base_io.hpp>
#include <rtc_base/platform_thread.h>

namespace ntgcalls::io {

    class ThreadedIO: public virtual BaseIO {
        std::vector<webrtc::PlatformThread> threads_;

    protected:
        void spawn(std::function<void()> loop, const std::string& name);

        void join();

    public:
        explicit ThreadedIO(media::BaseSink* sink);
    };

} // ntgcalls::io
