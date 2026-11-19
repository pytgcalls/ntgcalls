//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <ntgcalls/io/base_writer.hpp>
#include <ntgcalls/io/threaded_io.hpp>

namespace ntgcalls::io {

    class ThreadedWriter: public BaseWriter, public ThreadedIO {
        std::mutex mutex_;
        std::condition_variable cv_;
        std::queue<std::pair<bytes::unique_binary, size_t>> queue_;

        void run();

    protected:
        void push(bytes::unique_binary data, size_t size);

        virtual std::optional<std::pair<bytes::unique_binary, size_t>> idle_data();

        virtual void write(const bytes::unique_binary& data, size_t size) = 0;

    public:
        explicit ThreadedWriter(media::BaseSink* sink);

        void open() override;

        void close();

        virtual void flush() {}
    };

} // ntgcalls::io
