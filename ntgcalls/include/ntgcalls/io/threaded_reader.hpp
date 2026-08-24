//
// Created by Lauren on 28/09/24.
//

#pragma once

#include <condition_variable>
#include <ntgcalls/io/base_reader.hpp>
#include <rtc_base/platform_thread.h>
#include <wrtc/utils/sync_helper.hpp>

namespace ntgcalls::io {

    class ThreadedReader: public BaseReader, public wrtc::utils::SyncHelper {
        std::vector<webrtc::PlatformThread> buffer_threads_;
        size_t active_buffer_ = 0;
        size_t active_buffer_count_ = 0;
        std::condition_variable cv_;
        std::mutex mtx_;

    public:
        explicit ThreadedReader(media::BaseSink* sink, size_t thread_count = 2);

        void close();

        bool set_enabled(bool enable) override;

    protected:
        int64_t read_chunks_ = 0;

        void run(const std::function<bytes::unique_binary(int64_t)>& read_callback);
    };

} // ntgcalls::io
