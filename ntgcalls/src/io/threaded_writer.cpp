//
// Created by Lauren on 20/11/26.
//

#include <ntgcalls/io/threaded_writer.hpp>

namespace ntgcalls::io {
    ThreadedWriter::ThreadedWriter(media::BaseSink* sink): BaseIO(sink), BaseWriter(sink), ThreadedIO(sink) {}

    void ThreadedWriter::open() {
        if (running_) return;
        running_ = true;
        spawn(
            [this] {
                run();
            },
            "ThreadedWriter"
        );
    }

    void ThreadedWriter::run() {
        const auto timeout = sink_->frame_time() + std::chrono::milliseconds(20);
        while (true) {
            std::optional<std::pair<bytes::unique_binary, size_t>> item;
            {
                std::unique_lock lock(mutex_);
                const auto ready = cv_.wait_for(lock, timeout, [this] {
                    return !queue_.empty() || !running_;
                });
                if (!queue_.empty()) {
                    item = std::move(queue_.front());
                    queue_.pop();
                } else if (!running_) {
                    break;
                } else if (!ready) {
                    lock.unlock();
                    item = idle_data();
                }
            }
            if (!item || item->second == 0) {
                continue;
            }
            try {
                write(item->first, item->second);
            } catch (...) {
                running_ = false;
                (void) eof_callback_();
                break;
            }
        }
    }

    void ThreadedWriter::push(bytes::unique_binary data, const size_t size) {
        {
            const std::lock_guard lock(mutex_);
            queue_.emplace(std::move(data), size);
        }
        cv_.notify_one();
    }

    std::optional<std::pair<bytes::unique_binary, size_t>> ThreadedWriter::idle_data() {
        return std::nullopt;
    }

    void ThreadedWriter::close() {
        eof_callback_ = nullptr;
        {
            const std::lock_guard lock(mutex_);
            running_ = false;
        }
        cv_.notify_all();
        join();
    }
} // ntgcalls::io
