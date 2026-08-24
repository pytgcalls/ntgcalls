//
// Created by Lauren on 07/10/24.
//

#include <ntgcalls/io/threaded_audio_mixer.hpp>

namespace ntgcalls::io {
    ThreadedAudioMixer::ThreadedAudioMixer(media::BaseSink* sink): AudioMixer(sink) {}

    ThreadedAudioMixer::~ThreadedAudioMixer() {
        eof_callback_ = nullptr;
        const bool was_running = running_;
        if (running_) {
            {
                const std::lock_guard lock(mtx_);
                running_ = false;
            }
            cv_.notify_all();
        }
        if (was_running) thread_.Finalize();
    }

    void ThreadedAudioMixer::open() {
        if (running_) return;
        running_ = true;
        auto frame_size = sink_->frame_size();
        auto frame_time = sink_->frame_time();
        thread_ = webrtc::PlatformThread::SpawnJoinable(
            [this, frame_size, frame_time] {
                while (running_) {
                    std::unique_lock lock(mtx_);
                    const auto ok = cv_.wait_for(lock, frame_time + std::chrono::milliseconds(20), [this] {
                        const std::lock_guard queue_lock(queue_mutex_);
                        return !queue_.empty() || !running_;
                    });
                    if (!running_) {
                        break;
                    }
                    try {
                        if (ok) {
                            const std::lock_guard queue_lock(queue_mutex_);
                            write(queue_.front());
                            queue_.pop();
                        } else {
                            write(bytes::make_unique_binary(frame_size));
                        }
                    } catch (...) {
                        running_ = false;
                        break;
                    }
                }
                (void) eof_callback_();
            },
            "ThreadedMixer",
            webrtc::ThreadAttributes().SetPriority(webrtc::ThreadPriority::kRealtime)
        );
    }

    void ThreadedAudioMixer::on_data(bytes::unique_binary data) {
        const std::lock_guard queue_lock(queue_mutex_);
        queue_.push(std::move(data));
        cv_.notify_one();
    }
} // ntgcalls::io
