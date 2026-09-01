//
// Created by Lauren on 02/05/25.
//

#pragma once
#include <atomic>
#include <chrono>
#include <api/media_types.h>
#include <rtc_base/platform_thread.h>
#include <wrtc/models/media_segment.hpp>
#include <wrtc/utils/synchronized_callback.hpp>
#include <wrtc/utils/sync_helper.hpp>

namespace wrtc::interfaces::mtproto {

    using namespace std::chrono_literals;

    class ThreadBuffer {
    public:
        enum class RequestType {
            RequestSegments,
            RemoveSegment,
        };

        explicit ThreadBuffer(
            const std::function<void(webrtc::MediaType, models::MediaSegment*, std::chrono::milliseconds)>& frame_callback,
            const std::function<models::MediaSegment*()>& get_segment_callback,
            const std::function<void(RequestType)>& request_callback
        );

        ~ThreadBuffer();

    private:
        std::mutex mutex_;
        models::MediaSegment* last_segment_ = nullptr;
        int check_sync_count_ = 0;
        std::atomic_bool running_ = true;
        std::unique_ptr<utils::SyncHelper> audio_sync_, video_sync_;
        std::chrono::milliseconds audio_consumed_time_ = 0ms, video_consumed_time_ = 0ms;
        std::vector<webrtc::PlatformThread> threads_;
        std::function<void(RequestType)> request_callback_;
        std::function<models::MediaSegment*()> get_segment_callback_;
        std::function<void(webrtc::MediaType, models::MediaSegment*, std::chrono::milliseconds)> frame_callback_;

        void start_thread(webrtc::MediaType media_type);

        void check_segments_sync();

        models::MediaSegment* get_segment_sync(webrtc::MediaType media_type);
    };

} // wrtc::interfaces::mtproto
