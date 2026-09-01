//
// Created by Lauren on 28/09/24.
//

#include <algorithm>
#include <thread>
#include <ntgcalls/io/threaded_reader.hpp>

namespace ntgcalls::io {
    ThreadedReader::ThreadedReader(media::BaseSink* sink, const size_t thread_count): BaseReader(sink), SyncHelper(sink->frame_time()) {
        buffer_threads_.reserve(thread_count);
    }

    void ThreadedReader::close() {
        {
            const std::lock_guard lock(mtx_);
            data_callback_ = nullptr;
            eof_callback_ = nullptr;
            running_ = false;
            cv_.notify_all();
        }
        for (auto& thread : buffer_threads_) {
            thread.Finalize();
        }
    }

    void ThreadedReader::run(const std::function<bytes::unique_binary(int64_t)>& read_callback) {
        if (running_) return;
        const auto buffer_count = buffer_threads_.capacity();
        running_ = true;
        synchronize_time();
        const auto frame_time = sink_->frame_time();
        for (size_t i = 0; i < buffer_count; ++i) {
            buffer_threads_.push_back(
                webrtc::PlatformThread::SpawnJoinable(
                    [this, i, buffer_count, frame_size = sink_->frame_size(), max_buffer_size = std::max<int64_t>(1, std::chrono::seconds(1) / frame_time / 10), read_callback] {
                        active_buffer_count_++;
                        std::vector<bytes::unique_binary> frames;
                        frames.reserve(max_buffer_size);
                        while (running_) {
                            try {
                                std::unique_lock lock(mtx_);
                                auto data = read_callback(frame_size * max_buffer_size);
                                lock.unlock();
                                frames.clear();
                                for (size_t j = 0; j < max_buffer_size; j++) {
                                    const size_t offset = j * frame_size;
                                    auto chunk = bytes::make_unique_binary(frame_size);
                                    std::memcpy(chunk.get(), data.get() + offset, frame_size);
                                    frames.push_back(std::move(chunk));
                                }
                            } catch (...) {
                                const std::lock_guard lock(mtx_);
                                running_ = false;
                                cv_.notify_all();
                                break;
                            }
                            {
                                std::unique_lock lock(mtx_);
                                cv_.wait(lock, [this, i] {
                                    return !running_ || (active_buffer_ == i && status_);
                                });
                            }
                            if (!running_) break;
                            for (auto& chunk : frames) {
                                if (!running_) break;
                                data_callback_(std::move(chunk), {});
                                wait_next_frame();
                            }
                            {
                                const std::lock_guard lock(mtx_);
                                active_buffer_ = (active_buffer_ + 1) % buffer_count;
                                cv_.notify_all();
                            }
                        }
                        const std::lock_guard lock(mtx_);
                        active_buffer_count_--;
                        if (active_buffer_count_ == 0) {
                            (void) eof_callback_();
                        }
                    },
                    "ThreadedReader_" + std::to_string(buffer_count),
                    webrtc::ThreadAttributes().SetPriority(webrtc::ThreadPriority::kRealtime)
                )
            );
        }
    }

    bool ThreadedReader::set_enabled(const bool enable) {
        const std::lock_guard lock(mtx_);
        const auto res = BaseReader::set_enabled(enable);
        cv_.notify_all();
        return res;
    }
} // ntgcalls::io
