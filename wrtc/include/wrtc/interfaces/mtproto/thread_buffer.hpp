//
// Created by Laky64 on 02/05/25.
//

#pragma once
#include <atomic>
#include <chrono>
#include <api/media_types.h>
#include <rtc_base/platform_thread.h>
#include <wrtc/utils/sync_helper.hpp>
#include <wrtc/models/media_segment.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace wrtc {

    using namespace std::chrono_literals;

    class ThreadBuffer {
    public:
        enum class RequestType {
            RequestSegments,
            RemoveSegment,
        };

        explicit ThreadBuffer(
            const std::function<void(webrtc::MediaType, MediaSegment*, std::chrono::milliseconds)>& frameCallback,
            const std::function<MediaSegment*()>& getSegmentCallback,
            const std::function<void(RequestType)>& requestCallback
        );

        ~ThreadBuffer();

    private:
        std::mutex mutex;
        MediaSegment* lastSegment = nullptr;
        int checkSyncCount = 0;
        std::atomic_bool running = true;
        std::unique_ptr<SyncHelper> audioSync, videoSync;
        std::chrono::milliseconds audioConsumedTime = 0ms, videoConsumedTime = 0ms;
        std::vector<rtc::PlatformThread> threads;
        std::function<void(RequestType)> requestCallback;
        std::function<MediaSegment*()> getSegmentCallback;
        std::function<void(webrtc::MediaType, MediaSegment*, std::chrono::milliseconds)> frameCallback;

        void startThread(webrtc::MediaType mediaType);

        void checkSegmentsSync();

        MediaSegment* getSegmentSync(webrtc::MediaType mediaType);
    };

} // wrtc
