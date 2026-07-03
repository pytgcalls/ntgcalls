//
// Created by Lauren on 02/05/25.
//

#include <wrtc/interfaces/mtproto/thread_buffer.hpp>

namespace wrtc::interfaces::mtproto {
    ThreadBuffer::ThreadBuffer(
        const std::function<void(webrtc::MediaType, models::MediaSegment*, std::chrono::milliseconds)>& frame_callback,
        const std::function<models::MediaSegment*()>& get_segment_callback,
        const std::function<void(RequestType)>& request_callback
    ) : request_callback_(request_callback), get_segment_callback_(get_segment_callback), frame_callback_(frame_callback) {
        const auto current_time = std::chrono::steady_clock::now();
        audio_sync_ = std::make_unique<utils::SyncHelper>(10ms);
        video_sync_ = std::make_unique<utils::SyncHelper>(8ms);
        audio_sync_->synchronize_time(current_time);
        video_sync_->synchronize_time(current_time);
        start_thread(webrtc::MediaType::AUDIO);
        start_thread(webrtc::MediaType::VIDEO);
    }

    ThreadBuffer::~ThreadBuffer() {
        running_ = false;
        for (auto& thread : threads_) {
            thread.Finalize();
        }
        frame_callback_ = nullptr;
        get_segment_callback_ = nullptr;
        request_callback_ = nullptr;
        audio_sync_ = nullptr;
        video_sync_ = nullptr;
    }

    void ThreadBuffer::start_thread(webrtc::MediaType media_type) {
        threads_.push_back(
            webrtc::PlatformThread::SpawnJoinable(
                [this, media_type] {
                    while (running_) {
                        std::unique_lock lock(mutex_);
                        if (const auto segment = get_segment_sync(media_type)) {
                            lock.unlock();
                            frame_callback_(media_type, segment, media_type == webrtc::MediaType::AUDIO ? audio_consumed_time_ : video_consumed_time_);
                            lock.lock();
                        }
                        check_segments_sync();
                        lock.unlock();
                        if (media_type == webrtc::MediaType::AUDIO) {
                            audio_sync_->wait_next_frame();
                        } else {
                            video_sync_->wait_next_frame();
                        }
                    }
                },
                "ThreadBuffer",
                webrtc::ThreadAttributes().SetPriority(webrtc::ThreadPriority::kRealtime)
            )
        );
    }

    void ThreadBuffer::check_segments_sync() {
        check_sync_count_++;
        if (check_sync_count_ == threads_.size()) {
            check_sync_count_ = 0;
            request_callback_(RequestType::RequestSegments);
        }
    }

    models::MediaSegment* ThreadBuffer::get_segment_sync(const webrtc::MediaType media_type) {
        if (audio_consumed_time_ >= 1s && video_consumed_time_ >= 1s) {
            audio_consumed_time_ = 0ms;
            video_consumed_time_ = 0ms;
            last_segment_ = nullptr;
            request_callback_(RequestType::RemoveSegment);
        }

        if (audio_consumed_time_ == 0ms && video_consumed_time_ == 0ms || !last_segment_) {
            last_segment_ = get_segment_callback_();
        }

        const auto consume = [&](auto& time_consumed, auto increment) -> models::MediaSegment* {
            if (time_consumed >= 1s) return nullptr;
            if (last_segment_) time_consumed += increment;
            return last_segment_;
        };

        switch (media_type) {
        case webrtc::MediaType::AUDIO:
            return consume(audio_consumed_time_, 10ms);
        case webrtc::MediaType::VIDEO:
            return consume(video_consumed_time_, 8ms);
        default:
            return nullptr;
        }
    }
} // wrtc::interfaces::mtproto