//
// Created by Laky64 on 12/04/25.
//

#pragma once
#include <atomic>
#include <shared_mutex>
#include <rtc_base/thread.h>
#include <wrtc/models/audio_frame.hpp>
#include <wrtc/models/media_segment.hpp>
#include <wrtc/models/segment_part_request.hpp>
#include <wrtc/utils/synchronized_callback.hpp>
#include <wrtc/interfaces/mtproto/thread_buffer.hpp>

namespace wrtc {

    class MTProtoStream: public std::enable_shared_from_this<MTProtoStream> {
        struct AudioBuffer {
            uint32_t ssrc;
            std::vector<int16_t> data;
            size_t sampleRate = 0;
            int channels = 0;
        };

        struct VideoChannel {
            uint32_t ssrc;
            bool isScreenCast;
            MediaSegment::Quality quality;
        };

        bool isRtmp;
        std::atomic_bool audioIncoming = false;
        std::atomic_bool cameraIncoming = false;
        std::atomic_bool screenIncoming = false;
        bool isWaitingCurrentTime = false;
        const int segmentBufferDuration = 2000;
        const int segmentDuration = 1000;
        int nextPendingRequestTimeDelayTaskId = 0;
        int pendingRequestTimeDelayTaskId = 0;
        int64_t nextSegmentTimestamp = -1;
        int64_t serverTimeMs = 0;
        int64_t serverTimeMsGotAt = 0;
        rtc::Thread* mediaThread;
        std::atomic_bool running;

        AudioStreamingPartPersistentDecoder persistentAudioDecoder;
        std::optional<int> waitForBufferedMillisecondsBeforeRendering;
        std::map<int64_t, std::unique_ptr<MediaSegment>> segments;
        std::map<std::string, VideoChannel> videoChannels;
        std::map<std::string, int32_t> currentEndpointMapping;
        std::map<std::string, std::unique_ptr<VideoStreamingSharedState>> sharedVideoState;

        synchronized_callback<void> requestCurrentTimeCallback;
        synchronized_callback<int> updateAudioSourceCountCallback;
        synchronized_callback<std::unique_ptr<AudioFrame>> audioFrameCallback;
        synchronized_callback<uint32_t, bool, std::unique_ptr<webrtc::VideoFrame>> videoFrameCallback;
        synchronized_callback<SegmentPartRequest> requestBroadcastPartCallback;

        std::shared_mutex segmentMutex;
        std::unique_ptr<ThreadBuffer> threadBuffer;

        std::map<int64_t, MediaSegment*> filterSegments(MediaSegment::Status status) const;

        void render();

        int64_t getAvailableBufferDuration() const;

        void requestSegmentsIfNeeded();

        void checkPendingSegments();

        void discardAllPendingSegments();

        static void cancelPendingVideoQualityUpdate(MediaSegment::Video* segment);

        void checkPendingVideoQualityUpdate();

        void requestPendingVideoQualityUpdate(int64_t segmentId, int32_t partID, MediaSegment::Video* segment, int64_t timestamp);

    public:
        explicit MTProtoStream(rtc::Thread* mediaThread, bool isRtmp);

        void connect();

        void close();

        void sendBroadcastTimestamp(int64_t timestamp);

        void sendBroadcastPart(int64_t segmentID, int32_t partID, MediaSegment::Part::Status status, bool qualityUpdate, std::optional<bytes::binary> data);

        void addIncomingVideo(const std::string& endpoint, uint32_t ssrc, bool isScreenCast);

        bool removeIncomingVideo(const std::string& endpoint);

        void enableAudioIncoming(bool enable);

        void enableVideoIncoming(bool enable, bool isScreenCast);

        void onRequestBroadcastTime(const std::function<void()>& callback);

        void onRequestBroadcastPart(const std::function<void(SegmentPartRequest)>& callback);

        void onAudioFrame(const std::function<void(std::unique_ptr<AudioFrame>)>& callback);

        void onVideoFrame(const std::function<void(uint32_t, bool, std::unique_ptr<webrtc::VideoFrame>)>& callback);

        void onUpdateAudioSourceCount(const std::function<void(int)>& callback);
    };
} // wrtc
