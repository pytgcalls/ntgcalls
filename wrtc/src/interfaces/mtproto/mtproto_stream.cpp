//
// Created by Laky64 on 12/04/25.
//

#include <ranges>
#include <variant>
#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>
#include <wrtc/exceptions.hpp>
#include <wrtc/interfaces/mtproto/mtproto_stream.hpp>

namespace wrtc {

    MTProtoStream::MTProtoStream(rtc::Thread* mediaThread, const bool isRtmp) : isRtmp(isRtmp), mediaThread(mediaThread){}

    void MTProtoStream::connect() {
        if (running) {
            throw RTCException("MTProto Connection already made");
        }
        running = true;
        serverTimeMs = rtc::TimeUTCMillis();
        serverTimeMsGotAt = rtc::TimeMillis();
        beginRenderTimer(0);
    }

    void MTProtoStream::close() {
        audioFrameCallback = nullptr;
        videoFrameCallback = nullptr;
        requestCurrentTimeCallback = nullptr;
        requestBroadcastPartCallback = nullptr;
        updateAudioSourceCountCallback = nullptr;
        isWaitingCurrentTime = false;
        running = false;
        mediaThread->BlockingCall([&] {});
    }

    void MTProtoStream::sendBroadcastTimestamp(const int64_t timestamp) {
        std::weak_ptr weak(shared_from_this());
        mediaThread->PostTask([weak, timestamp]{
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->isWaitingCurrentTime = false;
            int64_t adjustedTimestamp = 0;
            if (timestamp > 0) {
                adjustedTimestamp = timestamp / strong->segmentDuration * strong->segmentDuration - strong->segmentBufferDuration;
            }
            if (adjustedTimestamp <= 0) {
                int taskId = strong->nextPendingRequestTimeDelayTaskId;
                strong->pendingRequestTimeDelayTaskId = taskId;
                strong->nextPendingRequestTimeDelayTaskId++;

                strong->mediaThread->PostDelayedTask([weak, taskId] {
                   const auto strongMedia = weak.lock();
                   if (!strongMedia) {
                       return;
                   }
                   if (strongMedia->pendingRequestTimeDelayTaskId != taskId) {
                       return;
                   }
                   strongMedia->pendingRequestTimeDelayTaskId = 0;
                   strongMedia->requestSegmentsIfNeeded();
               }, webrtc::TimeDelta::Millis(1000));
            } else {
                strong->nextSegmentTimestamp = adjustedTimestamp;
                strong->requestSegmentsIfNeeded();
            }
        });
    }

    void MTProtoStream::sendBroadcastPart(const int64_t segmentID, const int32_t partID, const MediaSegment::Part::Status status, const bool qualityUpdate, std::optional<bytes::binary> data) {
        std::weak_ptr weak(shared_from_this());
        mediaThread->PostTask([weak, segmentID, partID, status, qualityUpdate, data = std::move(data)] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }

            bool foundPart = false;
            if (strong->segments.contains(segmentID)) {
                if (qualityUpdate) {
                    foundPart = strong->segments[segmentID]->video.size() > partID &&
                        strong->segments[segmentID]->video[partID]->qualityUpdatePart;
                } else {
                    foundPart = strong->segments[segmentID]->parts.size() > partID;
                }
            }

            if (!foundPart) {
                RTC_LOG(LS_WARNING) << "Part " << partID << " not found in segment " << segmentID;
                return;
            }

            const auto &segment = strong->segments[segmentID];
            MediaSegment::Part* part;
            if (qualityUpdate) {
                part = segment->video[partID]->qualityUpdatePart.get();
            } else {
                part = segment->parts[partID].get();
            }
            const auto responseTimestamp = rtc::TimeMillis();
            const auto responseTimestampMilliseconds = static_cast<int64_t>(static_cast<double>(responseTimestamp) * 1000.0);
            const auto responseTimestampBoundary = responseTimestampMilliseconds / strong->segmentDuration * strong->segmentDuration;

            part->status = status;
            switch (status) {
            case MediaSegment::Part::Status::Success:
                part->data = data;
                if (strong->nextSegmentTimestamp == -1) {
                    strong->nextSegmentTimestamp = part->timestampMilliseconds + strong->segmentDuration;
                }
                strong->checkPendingSegments();
                if (qualityUpdate) {
                    segment->video[partID]->part = std::make_unique<VideoStreamingPart>(std::move(part->data.value()));
                    segment->video[partID]->qualityUpdatePart = nullptr;
                }
                break;
            case MediaSegment::Part::Status::NotReady:
                if (segment->timestamp == 0) {
                    strong->nextSegmentTimestamp = responseTimestampBoundary;
                    strong->discardAllPendingSegments();
                    strong->requestSegmentsIfNeeded();
                    strong->checkPendingSegments();
                } else {
                    part->minRequestTimestamp = rtc::TimeMillis() + 100;
                    strong->checkPendingSegments();
                }
                break;
            case MediaSegment::Part::Status::ResyncNeeded:
                strong->nextSegmentTimestamp = responseTimestampBoundary;
                strong->discardAllPendingSegments();
                strong->requestSegmentsIfNeeded();
                strong->checkPendingSegments();
                break;
            default:
                throw RTCException("Invalid part status");
            }
        });
    }

    void MTProtoStream::addIncomingVideo(const std::string& endpoint, const uint32_t ssrc, bool isScreenCast) {
        std::weak_ptr weak(shared_from_this());
        mediaThread->BlockingCall([weak, endpoint, ssrc, isScreenCast] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->videoChannels[endpoint] = VideoChannel(
                ssrc,
                isScreenCast,
                isScreenCast ? MediaSegment::Quality::Full : MediaSegment::Quality::Medium
            );
            strong->checkPendingVideoQualityUpdate();
        });
    }

    bool MTProtoStream::removeIncomingVideo(const std::string& endpoint) {
        std::weak_ptr weak(shared_from_this());
        return mediaThread->BlockingCall([weak, endpoint] {
            const auto strong = weak.lock();
            if (!strong) {
                return false;
            }
            if (strong->videoChannels.contains(endpoint)) {
                strong->videoChannels.erase(endpoint);
                strong->checkPendingVideoQualityUpdate();
                return true;
            }
            return false;
        });
    }

    void MTProtoStream::enableAudioIncoming(const bool enable) {
        audioIncoming = enable;
    }

    void MTProtoStream::enableVideoIncoming(const bool enable, const bool isScreenCast) {
        if (isScreenCast) {
            screenIncoming = enable;
        } else {
            cameraIncoming = enable;
        }
    }

    void MTProtoStream::onRequestBroadcastTime(const std::function<void()>& callback) {
        requestCurrentTimeCallback = callback;
    }

    void MTProtoStream::onRequestBroadcastPart(const std::function<void(SegmentPartRequest)>& callback) {
        requestBroadcastPartCallback = callback;
    }

    void MTProtoStream::onAudioFrame(const std::function<void(std::unique_ptr<AudioFrame>)>& callback){
        audioFrameCallback = callback;
    }

    void MTProtoStream::onVideoFrame(const std::function<void(uint32_t, bool, std::unique_ptr<webrtc::VideoFrame>)>& callback) {
        videoFrameCallback = callback;
    }

    void MTProtoStream::onUpdateAudioSourceCount(const std::function<void(int count)>& callback) {
        updateAudioSourceCountCallback = callback;
    }

    std::map<int64_t, MediaSegment*> MTProtoStream::filterSegments(const MediaSegment::Status status) const {
        std::map<int64_t, MediaSegment*> availableSegments;
        for (const auto& [fst, snd] : segments) {
            if (snd->status == status) {
                availableSegments[fst] = snd.get();
            }
        }
        return availableSegments;
    }

    void MTProtoStream::render() {
        const int64_t absoluteTimestamp = rtc::TimeMillis();
        while (true) {
            if (waitForBufferedMillisecondsBeforeRendering) {
                if (getAvailableBufferDuration() < waitForBufferedMillisecondsBeforeRendering.value()) {
                    break;
                }
                waitForBufferedMillisecondsBeforeRendering = std::nullopt;
            }
            const auto availableSegments = filterSegments(MediaSegment::Status::Ready);
            if (availableSegments.empty()) {
                playbackReferenceTimestamp = 0;
                waitForBufferedMillisecondsBeforeRendering = segmentBufferDuration + segmentDuration;
                break;
            }

            if (playbackReferenceTimestamp == 0) {
                playbackReferenceTimestamp = absoluteTimestamp;
            }

            const double relativeTimestamp = static_cast<double>(absoluteTimestamp - playbackReferenceTimestamp) / 1000.0;

            const auto segment = availableSegments.begin()->second;
            const double renderSegmentDuration = static_cast<double>(segment->duration) / 1000.0;

            std::set<std::string> usedEndpoints;
            for (const auto &videoSegment : segment->video) {
                videoSegment->isPlaying = true;
                cancelPendingVideoQualityUpdate(videoSegment.get());
                if (auto endpointId = videoSegment->part->getActiveEndpointId(); endpointId.has_value() && videoChannels.contains(endpointId.value())) {
                    const auto videoChannel = videoChannels[endpointId.value()];
                    const auto isScreenCast = videoChannel.isScreenCast;
                    const auto ssrc = videoChannel.ssrc;
                    if ((isScreenCast && screenIncoming) || (!isScreenCast && cameraIncoming)) {
                        if (!sharedVideoState.contains(endpointId.value())) {
                            sharedVideoState[endpointId.value()] = std::make_unique<VideoStreamingSharedState>();
                        }
                        usedEndpoints.insert(endpointId.value());
                        if (const auto frame = videoSegment->part->getFrameAtRelativeTimestamp(sharedVideoState[endpointId.value()].get(), relativeTimestamp)) {
                            if (videoSegment->lastFramePts != frame->pts) {
                                videoSegment->lastFramePts = frame->pts;
                                auto videoFrame = std::make_unique<webrtc::VideoFrame>(frame->frame);
                                videoFrame->set_timestamp_us(static_cast<int64_t>(frame->pts * 1000) + playbackReferenceTimestamp);
                                videoFrameCallback(
                                    ssrc,
                                    isScreenCast,
                                    std::move(videoFrame)
                                );
                            }
                        }
                    }
                }
            }

            for (auto it = sharedVideoState.begin(); it != sharedVideoState.end();) {
                if (!usedEndpoints.contains(it->first) && !videoChannels.contains(it->first)) {
                    it = sharedVideoState.erase(it);
                } else {
                    ++it;
                }
            }

            if (segment->audio && audioIncoming) {
                while (true) {
                    const auto audioChannels = segment->audio->get10msPerChannel(segment->audioDecoder);
                    if (audioChannels.empty()) {
                        break;
                    }
                    (void) updateAudioSourceCountCallback(static_cast<int>(audioChannels.size()));
                    for (const auto & [ssrc, pcmData, numSamples] : audioChannels) {
                        auto frame = std::make_unique<AudioFrame>(ssrc);
                        frame->channels = pcmData.size() / numSamples;
                        frame->sampleRate = numSamples * 100;
                        frame->data = pcmData.data();
                        frame->size = numSamples * frame->channels * sizeof(int16_t);
                        (void) audioFrameCallback(std::move(frame));
                    }
                }
            }

            if (relativeTimestamp >= renderSegmentDuration) {
                playbackReferenceTimestamp += segment->duration;
                segments.erase(segments.begin());
            }
            break;
        }

        requestSegmentsIfNeeded();
        checkPendingSegments();
    }

    int64_t MTProtoStream::getAvailableBufferDuration() const {
        int64_t result = 0;
        for (const auto segment : filterSegments(MediaSegment::Status::Ready) | std::views::values) {
            result += segment->duration;
        }
        return result;
    }

    void MTProtoStream::requestSegmentsIfNeeded() {
        while (true) {
            if (nextSegmentTimestamp == -1) {
                if (!isWaitingCurrentTime && pendingRequestTimeDelayTaskId == 0) {
                    isWaitingCurrentTime = true;
                    if (isRtmp) {
                        (void) requestCurrentTimeCallback();
                    } else {
                        sendBroadcastTimestamp(serverTimeMs + (rtc::TimeMillis() - serverTimeMsGotAt));
                    }
                }
                break;
            }
            int64_t availableAndRequestedSegmentsDuration = 0;
            availableAndRequestedSegmentsDuration += getAvailableBufferDuration();
            availableAndRequestedSegmentsDuration += static_cast<int64_t>(filterSegments(MediaSegment::Status::Pending).size()) * segmentDuration;

            if (availableAndRequestedSegmentsDuration > segmentBufferDuration) {
                break;
            }

            auto pendingSegment = std::make_unique<MediaSegment>();
            pendingSegment->timestamp = nextSegmentTimestamp;

            if (nextSegmentTimestamp != -1) {
                nextSegmentTimestamp += segmentDuration;
            }

            auto audioPart = std::make_unique<MediaSegment::Part>(MediaSegment::Part::Audio());
            pendingSegment->parts.push_back(std::move(audioPart));

            for (const auto &[endpoint, videoChannel] : videoChannels) {
                if (!currentEndpointMapping.contains(endpoint)) {
                    continue;
                }
                const int32_t channelId = currentEndpointMapping[endpoint] + 1;
                auto videoPart = std::make_unique<MediaSegment::Part>(MediaSegment::Part::Video(channelId, videoChannel.quality));
                pendingSegment->parts.push_back(std::move(videoPart));
            }

            segments[nextSegmentTimestamp] = std::move(pendingSegment);

            if (nextSegmentTimestamp == -1) {
                break;
            }
        }
    }

    void MTProtoStream::checkPendingSegments() {
        const auto absoluteTimestamp = rtc::TimeMillis();
        int64_t minDelayedRequestTimeout = INT_MAX;

        bool shouldRequestMoreSegments = false;
        int i = 0;
        for (auto& [segmentID, pendingSegment] : filterSegments(MediaSegment::Status::Pending)) {
            const auto segmentTimestamp = pendingSegment->timestamp;
            bool allPartsDone = true;
            for (int partID = 0; partID < pendingSegment->parts.size(); partID++) {
                const auto part = pendingSegment->parts[partID].get();
                if (!part->data) {
                    allPartsDone = false;
                }
                if (!part->data && part->status != MediaSegment::Part::Status::Downloading) {
                    if (part->minRequestTimestamp != 0) {
                        if (part->minRequestTimestamp > absoluteTimestamp) {
                            minDelayedRequestTimeout = std::min(minDelayedRequestTimeout, part->minRequestTimestamp - absoluteTimestamp);
                            continue;
                        }
                    }
                    auto videoQuality = MediaSegment::Quality::None;
                    int32_t videoChannelId = 0;
                    const auto typeData = &part->typeData;

                    if (const auto video = std::get_if<MediaSegment::Part::Video>(typeData)) {
                        videoQuality = video->quality;
                        videoChannelId = video->channelId;
                    }

                    const auto requested = requestBroadcastPartCallback({
                        segmentID,
                        partID,
                        SegmentPartRequest::DEFAULT_SIZE,
                        segmentTimestamp,
                        false,
                        videoChannelId,
                        videoQuality
                    });

                    if (requested) {
                        part->status = MediaSegment::Part::Status::Downloading;
                        part->timestampMilliseconds = segmentTimestamp;
                    }
                }
            }

            if (allPartsDone && i == 0) {
                pendingSegment->duration = segmentDuration;
                pendingSegment->status = MediaSegment::Status::Ready;
                for (const auto& part : pendingSegment->parts) {
                    if (const auto typeData = &part->typeData; std::get_if<MediaSegment::Part::Audio>(typeData)) {
                        pendingSegment->audio = std::make_unique<AudioStreamingPart>(std::move(part->data.value()), "ogg", false);
                        currentEndpointMapping = pendingSegment->audio->getEndpointMapping();
                    } else if (const auto videoData = std::get_if<MediaSegment::Part::Video>(typeData)) {
                        auto videoSegment = std::make_unique<MediaSegment::Video>();
                        videoSegment->quality = videoData->quality;
                        if (part->data.value().empty()) {
                            RTC_LOG(LS_INFO) << "Video part " << pendingSegment->timestamp << " is empty";
                        }
                        videoSegment->part = std::make_unique<VideoStreamingPart>(std::move(part->data.value()));
                        pendingSegment->video.push_back(std::move(videoSegment));
                    }
                }
                pendingSegment->parts.clear();
                shouldRequestMoreSegments = true;
                i--;
            }
            i++;
        }

        if (minDelayedRequestTimeout < INT32_MAX) {
            std::weak_ptr weak(shared_from_this());
            mediaThread->PostDelayedTask([weak] {
                const auto strong = weak.lock();
                if (!strong) {
                    return;
                }
                strong->checkPendingSegments();
            }, webrtc::TimeDelta::Millis(std::max(static_cast<int32_t>(minDelayedRequestTimeout), 10)));
        }

        if (shouldRequestMoreSegments) {
            requestSegmentsIfNeeded();
        }
    }

    void MTProtoStream::beginRenderTimer(const int timeoutMs) {
        std::weak_ptr weak(shared_from_this());
        mediaThread->PostDelayedTask([weak]{
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->render();
            strong->beginRenderTimer(static_cast<int>(1.0 * 1000.0 / 120.0));
        }, webrtc::TimeDelta::Millis(timeoutMs));
    }

    void MTProtoStream::discardAllPendingSegments() {
        for (auto it = segments.begin(); it != segments.end(); ) {
            if (it->second->status == MediaSegment::Status::Pending) {
                it = segments.erase(it);
            } else {
                ++it;
            }
        }
    }

    void MTProtoStream::cancelPendingVideoQualityUpdate(MediaSegment::Video* segment) {
        if (!segment->qualityUpdatePart) {
            return;
        }
        segment->qualityUpdatePart = nullptr;
    }

    void MTProtoStream::checkPendingVideoQualityUpdate() {
        for (const auto & [endpointId, videoChannel] : videoChannels) {
            for (const auto &[segmentId, segment] : filterSegments(MediaSegment::Status::Ready)) {
                for (int partID = 0; partID < segment->video.size(); partID++) {
                    if (const auto video = segment->video[partID].get(); video->part->getActiveEndpointId() == endpointId) {
                        if (video->quality != videoChannel.quality) {
                            requestPendingVideoQualityUpdate(segmentId, partID, video, segment->timestamp);
                        }
                    }
                }
            }
        }
    }

    void MTProtoStream::requestPendingVideoQualityUpdate(int64_t segmentId, int32_t partID, MediaSegment::Video* segment, int64_t timestamp) {
        if (segment->isPlaying) {
            return;
        }

        if (const auto segmentEndpointId = segment->part->getActiveEndpointId(); !segmentEndpointId) {
            return;
        }

        std::optional<int32_t> updatedChannelId;
        std::optional<MediaSegment::Quality> updatedQuality;

        for (const auto & [endpointId, videoChannel] : videoChannels) {
            if (!currentEndpointMapping.contains(endpointId)) {
                continue;
            }

            updatedChannelId = currentEndpointMapping[endpointId] + 1;
            updatedQuality = videoChannel.quality;
        }

        if (updatedChannelId && updatedQuality) {
            if (segment->qualityUpdatePart) {
                const auto typeData = &segment->qualityUpdatePart->typeData;
                if (const auto videoData = std::get_if<MediaSegment::Part::Video>(typeData)) {
                    if (videoData->channelId == updatedChannelId.value() && videoData->quality == updatedQuality.value()) {
                        return;
                    }
                }
                cancelPendingVideoQualityUpdate(segment);
            }

            auto video = std::make_unique<MediaSegment::Part>(
                MediaSegment::Part::Video(updatedChannelId.value(), updatedQuality.value())
            );
            video->status = MediaSegment::Part::Status::Downloading;
            video->minRequestTimestamp = 0;
            segment->qualityUpdatePart = std::move(video);

            (void) requestBroadcastPartCallback({
                segmentId,
                partID,
                SegmentPartRequest::DEFAULT_SIZE,
                timestamp,
                true,
                updatedChannelId.value(),
                updatedQuality.value()
            });
        }
    }

} // wrtc