//
// Created by Lauren on 28/09/24.
//

#pragma once

#include <condition_variable>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/io/threaded_io.hpp>
#include <wrtc/utils/sync_helper.hpp>

namespace ntgcalls::io {

    class ThreadedReader: public BaseReader, public ThreadedIO, public wrtc::utils::SyncHelper {
        size_t buffer_count_;
        size_t active_buffer_ = 0;
        size_t next_read_ = 0;
        size_t active_buffer_count_ = 0;
        bool synchronized_ = false;
        std::condition_variable cv_;
        std::mutex mtx_;

        void run_buffer(size_t index, std::chrono::nanoseconds max_buffer_size);

    protected:
        struct Chunk {
            bytes::unique_binary data;
            size_t size;
            std::chrono::nanoseconds duration;
            wrtc::models::FrameData frame_data;
        };

        virtual std::vector<Chunk> read_frames(std::chrono::nanoseconds duration) = 0;

        virtual bytes::unique_binary read(int64_t size) = 0;

    public:
        explicit ThreadedReader(media::BaseSink* sink, size_t buffer_count = 2);

        void open() override;

        void close();

        bool set_enabled(bool enable) override;
    };

} // ntgcalls::io
