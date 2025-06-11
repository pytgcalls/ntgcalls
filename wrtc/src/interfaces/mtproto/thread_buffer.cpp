//
// Created by Laky64 on 02/05/25.
//

#include <wrtc/interfaces/mtproto/thread_buffer.hpp>

namespace wrtc {
    ThreadBuffer::ThreadBuffer(
        const std::function<void(webrtc::MediaType, MediaSegment*, std::chrono::milliseconds)>& frameCallback,
        const std::function<MediaSegment*()>& getSegmentCallback,
        const std::function<void(RequestType)>& requestCallback
    ) : requestCallback(requestCallback), getSegmentCallback(getSegmentCallback), frameCallback(frameCallback) {
        const auto currentTime = std::chrono::steady_clock::now();
        audioSync = std::make_unique<SyncHelper>(10ms);
        videoSync = std::make_unique<SyncHelper>(8ms);
        audioSync->synchronizeTime(currentTime);
        videoSync->synchronizeTime(currentTime);
        startThread(webrtc::MediaType::AUDIO);
        startThread(webrtc::MediaType::VIDEO);
    }

    ThreadBuffer::~ThreadBuffer() {
        running = false;
        for (auto& thread : threads) {
            thread.Finalize();
        }
        audioSync = nullptr;
        videoSync = nullptr;
    }

    void ThreadBuffer::startThread(webrtc::MediaType mediaType) {
        threads.push_back(
            webrtc::PlatformThread::SpawnJoinable(
                [this, mediaType] {
                    while (running) {
                        std::unique_lock lock(mutex);
                        if (const auto segment = getSegmentSync(mediaType)) {
                            lock.unlock();
                            frameCallback(mediaType, segment, mediaType == webrtc::MediaType::AUDIO ? audioConsumedTime : videoConsumedTime);
                            lock.lock();
                        }
                        checkSegmentsSync();
                        lock.unlock();
                        if (mediaType == webrtc::MediaType::AUDIO) {
                            audioSync->waitNextFrame();
                        } else {
                            videoSync->waitNextFrame();
                        }
                    }
                },
                "ThreadBuffer",
                webrtc::ThreadAttributes().SetPriority(webrtc::ThreadPriority::kRealtime)
            )
        );
    }

    void ThreadBuffer::checkSegmentsSync() {
        checkSyncCount++;
        if (checkSyncCount == threads.size()) {
            checkSyncCount = 0;
            requestCallback(RequestType::RequestSegments);
        }
    }

    MediaSegment* ThreadBuffer::getSegmentSync(const webrtc::MediaType mediaType) {
        if (audioConsumedTime >= 1s && videoConsumedTime >= 1s) {
            audioConsumedTime = 0ms;
            videoConsumedTime = 0ms;
            lastSegment = nullptr;
            requestCallback(RequestType::RemoveSegment);
        }

        if (audioConsumedTime == 0ms && videoConsumedTime == 0ms || !lastSegment) {
            lastSegment = getSegmentCallback();
        }

        const auto consume = [&](auto& timeConsumed, auto increment) -> MediaSegment* {
            if (timeConsumed >= 1s) return nullptr;
            if (lastSegment) timeConsumed += increment;
            return lastSegment;
        };

        switch (mediaType) {
        case webrtc::MediaType::AUDIO:
            return consume(audioConsumedTime, 10ms);
        case webrtc::MediaType::VIDEO:
            return consume(videoConsumedTime, 8ms);
        default:
            return nullptr;
        }
    }
} // wrtc