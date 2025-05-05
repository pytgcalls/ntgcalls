//
// Created by Laky64 on 28/09/24.
//

#pragma once

#include <condition_variable>
#include <ntgcalls/io/base_reader.hpp>
#include <wrtc/utils/sync_helper.hpp>
#include <rtc_base/platform_thread.h>

namespace ntgcalls {

    class ThreadedReader: public BaseReader, public wrtc::SyncHelper {
        std::vector<rtc::PlatformThread> bufferThreads;
        size_t activeBuffer = 0;
        size_t activeBufferCount = 0;
        uint64_t frameSent = 0;
        std::condition_variable cv;
        std::mutex mtx;

    public:
        explicit ThreadedReader(BaseSink *sink, size_t threadCount = 2);

        void close();

    protected:
        int64_t readChunks = 0;
        uint32_t uniqueID;

        void run(const std::function<bytes::unique_binary(int64_t)>& readCallback);

        bool set_enabled(bool status) override;
    };

} // ntgcalls
