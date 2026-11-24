//
// Created by Lauren on 28/09/24.
//

#include <algorithm>
#include <ntgcalls/io/threaded_reader.hpp>

namespace ntgcalls::io {
    ThreadedReader::ThreadedReader(media::BaseSink* sink, const size_t buffer_count): BaseIO(sink), BaseReader(sink), ThreadedIO(sink), SyncHelper(sink->frame_time()), buffer_count_(buffer_count) {}

    void ThreadedReader::close() {
        {
            const std::lock_guard lock(mtx_);
            data_callback_ = nullptr;
            eof_callback_ = nullptr;
            running_ = false;
            cv_.notify_all();
        }
        join();
    }

    void ThreadedReader::open() {
        if (running_) return;
        running_ = true;
        synchronized_ = false;
        active_buffer_ = 0;
        next_read_ = 0;
        constexpr auto kMaxBufferSize = std::chrono::milliseconds(100);
        for (size_t i = 0; i < buffer_count_; ++i) {
            spawn(
                [this, i, kMaxBufferSize] {
                    run_buffer(i, kMaxBufferSize);
                },
                "ThreadedReader_" + std::to_string(buffer_count_)
            );
        }
    }

    void ThreadedReader::run_buffer(const size_t index, const std::chrono::nanoseconds max_buffer_size) {
        {
            const std::lock_guard lock(mtx_);
            active_buffer_count_++;
        }
        std::vector<Chunk> frames;
        while (running_) {
            try {
                {
                    std::unique_lock lock(mtx_);
                    cv_.wait(lock, [this, index] {
                        return !running_ || next_read_ == index;
                    });
                    if (!running_) break;
                }
                frames = read_frames(max_buffer_size);
                {
                    const std::lock_guard lock(mtx_);
                    next_read_ = (next_read_ + 1) % buffer_count_;
                    cv_.notify_all();
                }
            } catch (...) {
                const std::lock_guard lock(mtx_);
                running_ = false;
                cv_.notify_all();
                break;
            }
            {
                std::unique_lock lock(mtx_);
                cv_.wait(lock, [this, index] {
                    return !running_ || (active_buffer_ == index && status_);
                });
                if (!synchronized_) {
                    synchronized_ = true;
                    synchronize_time();
                }
            }
            if (!running_) break;
            for (auto& [chunk, size, duration, frame_data] : frames) {
                if (!running_) break;
                data_callback_(std::move(chunk), size, frame_data);
                wait_next_frame(duration);
            }
            {
                const std::lock_guard lock(mtx_);
                active_buffer_ = (active_buffer_ + 1) % buffer_count_;
                cv_.notify_all();
            }
        }
        const std::lock_guard lock(mtx_);
        active_buffer_count_--;
        if (active_buffer_count_ == 0) {
            (void) eof_callback_();
        }
    }

    bool ThreadedReader::set_enabled(const bool enable) {
        const std::lock_guard lock(mtx_);
        const auto res = BaseReader::set_enabled(enable);
        cv_.notify_all();
        return res;
    }
} // ntgcalls::io
