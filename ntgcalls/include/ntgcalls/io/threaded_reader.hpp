//
// Created by Laky64 on 28/09/24.
//

#pragma once

#include <atomic>
#include <condition_variable>
#include <ntgcalls/io/base_reader.hpp>
#include <rtc_base/platform_thread.h>

namespace ntgcalls {

    class ThreadedReader: public BaseReader {
        std::vector<rtc::PlatformThread> bufferThreads;
        std::atomic_bool running, eof = false;
        size_t activeBuffer = 0;
        size_t activeBufferCount = 0;
        std::condition_variable cv;
        std::mutex mtx;
        std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;

    public:
        explicit ThreadedReader(BaseSink *sink, size_t bufferCount = 2);

        ~ThreadedReader() override;

    protected:
        int64_t readChunks = 0;

        virtual bytes::unique_binary read(int64_t size) = 0;

    public:
        void open() override;

        bool set_enabled(bool status) override;
    };

} // ntgcalls
