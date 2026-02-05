//
// Created by Laky64 on 12/04/25.
//

#include <ranges>
#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>
#include <wrtc/exceptions.hpp>
#include <wrtc/interfaces/mtproto/mtproto_stream.hpp>

namespace wrtc {

    MTProtoStream::MTProtoStream(webrtc::Thread* mediaThread, const bool isRtmp) : isRtmp(isRtmp), mediaThread(mediaThread) {}

    void MTProtoStream::connect() {
        if (running) {
            throw RTCException("MTProto Connection already made");
        }
        running = true;
        serverTimeMs = webrtc::TimeUTCMillis();
        serverTimeMsGotAt = webrtc::TimeMillis();
        render();
    }

    void MTProtoStream::close() {
        threadBuffer = nullptr;
        audioFrameCallback = nullptr;
        videoFrameCallback = nullptr;
        requestCurrentTimeCallback = nullptr;
        requestBroadcastPartCallback = nullptr;
        updateAudioSourceCountCallback = nullptr;
        running = false;
    }

    void MTProtoStream::sendBroadcastTimestamp(const int64_t timestamp) {
        std::weak_ptr weak(shared_from_this());
        mediaThread->PostTask([weak, timestamp]{
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            std::lock_guard lock(strong->segmentMutex);
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
                    std::lock_guard lockMedia(strongMedia->segmentMutex);
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

            std::lock_guard lock(strong->segmentMutex);
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
                return;
            }

            const auto &segment = strong->segments[segmentID];
            MediaSegment::Part* part;
            if (qualityUpdate) {
                part = segment->video[partID]->qualityUpdatePart.get();
            } else {
                part = segment->parts[partID].get();
            }
            const auto responseTimestamp = webrtc::TimeMillis();
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
                if (segment->timestamp == 0 && !strong->isRtmp) {
                    strong->nextSegmentTimestamp = responseTimestampBoundary;
                    strong->discardAllPendingSegments();
                    strong->requestSegmentsIfNeeded();
                    strong->checkPendingSegments();
                } else {
                    part->minRequestTimestamp = webrtc::TimeMillis() + 100;
                    strong->checkPendingSegments();
                }
                break;
            case MediaSegment::Part::Status::ResyncNeeded:
                if (strong->isRtmp) {
                    strong->nextSegmentTimestamp = -1;
                } else {
                    strong->nextSegmentTimestamp = responseTimestampBoundary;
                }
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
        if (isRtmp) {
            return;
        }
        std::lock_guard lock(segmentMutex);
        videoChannels[endpoint] = VideoChannel(
            ssrc,
            isScreenCast,
            isScreenCast ? MediaSegment::Quality::Full : MediaSegment::Quality::Medium
        );
        checkPendingVideoQualityUpdate();
    }

    bool MTProtoStream::removeIncomingVideo(const std::string& endpoint) {
        if (isRtmp) {
            return false;
        }
        std::lock_guard lock(segmentMutex);
        if (videoChannels.contains(endpoint)) {
            videoChannels.erase(endpoint);
            checkPendingVideoQualityUpdate();
            return true;
        }
        return false;
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
        std::weak_ptr weak(shared_from_this());
        threadBuffer = std::make_unique<ThreadBuffer>([weak] (const webrtc::MediaType mediaType, MediaSegment* segment, const std::chrono::milliseconds relativeTimestamp) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            std::shared_lock lock(strong->segmentMutex);
            if (mediaType == webrtc::MediaType::AUDIO) {
                if ((segment->audio || segment->unifiedAudio) && strong->audioIncoming) {
                    std::vector<AudioStreamingPartState::Channel> audioChannels;
                    if (strong->isRtmp) {
                        audioChannels = segment->unifiedAudio->getAudio10msPerChannel(strong->persistentAudioDecoder);
                    } else {
                        audioChannels = segment->audio->get10msPerChannel(segment->audioDecoder);
                    }

                    if (audioChannels.empty()) {
                        return;
                    }

                    std::map<uint32_t, std::unique_ptr<AudioBuffer>> interleavedAudioBySSRC;
                    for (const auto& [ssrc, pcmData] : audioChannels) {
                        uint32_t realSSRC = strong->isRtmp ? 1 : ssrc;
                        const size_t sampleCount = strong->isRtmp ? 480 : pcmData.size();

                        auto &prevBuffer = interleavedAudioBySSRC[realSSRC];
                        if (!prevBuffer) {
                            prevBuffer = std::make_unique<AudioBuffer>();
                            prevBuffer->ssrc = realSSRC;
                            prevBuffer->sampleRate = sampleCount * 100;
                        }

                        const int channelCount = prevBuffer->channels;
                        prevBuffer->channels++;

                        std::vector<int16_t> output;
                        output.reserve(sampleCount * channelCount);
                        for (size_t i = 0; i < sampleCount; ++i) {
                            for (int j = 0; j < channelCount; ++j) {
                                output.push_back(i < prevBuffer->data.size() ? prevBuffer->data[i] : static_cast<int16_t>(0));
                            }
                            output.push_back(i < pcmData.size() ? pcmData[i] : static_cast<int16_t>(0));
                        }
                        prevBuffer->data = std::move(output);
                    }
                    for (const auto& buffer : interleavedAudioBySSRC | std::views::values) {
                        auto frame = std::make_unique<AudioFrame>(buffer->ssrc);
                        frame->channels = buffer->channels;
                        frame->sampleRate = static_cast<int>(buffer->sampleRate);
                        frame->data = buffer->data.data();
                        frame->size = buffer->data.size() * sizeof(int16_t);
                        strong->audioFrameCallback(std::move(frame));
                    }
                }
            } else {
                std::set<std::string> usedEndpoints;
                for (const auto &videoSegment : segment->video) {
                    videoSegment->isPlaying = true;
                    std::optional<std::string> endpointId;

                    if (strong->isRtmp) {
                        endpointId = "unified";
                    } else {
                        cancelPendingVideoQualityUpdate(videoSegment.get());
                        endpointId = videoSegment->part->getActiveEndpointId();
                    }

                    if (endpointId.has_value() && (strong->videoChannels.contains(endpointId.value()) || strong->isRtmp)) {
                        bool isScreenCast = false;
                        uint32_t ssrc = 1;

                        if (!strong->isRtmp) {
                            const auto videoChannel = strong->videoChannels[endpointId.value()];
                            isScreenCast = videoChannel.isScreenCast;
                            ssrc = videoChannel.ssrc;
                        }

                        if ((isScreenCast && strong->screenIncoming) || (!isScreenCast && strong->cameraIncoming)) {
                            if (!strong->sharedVideoState.contains(endpointId.value())) {
                                strong->sharedVideoState[endpointId.value()] = std::make_unique<VideoStreamingSharedState>();
                            }
                            usedEndpoints.insert(endpointId.value());
                            if (const auto frame = videoSegment->part->getFrameAtRelativeTimestamp(
                                strong->sharedVideoState[endpointId.value()].get(),
                                static_cast<double>(relativeTimestamp.count()) / 1000.0
                            )) {
                                if (videoSegment->lastFramePts != frame->pts) {
                                    videoSegment->lastFramePts = frame->pts;
                                    auto videoFrame = std::make_unique<webrtc::VideoFrame>(frame->frame);
                                    const auto frameTimestamp = static_cast<int64_t>(frame->pts * 1000) + segment->timestamp;
                                    videoFrame->set_timestamp_us(frameTimestamp);
                                    strong->videoFrameCallback(
                                        ssrc,
                                        isScreenCast,
                                        std::move(videoFrame)
                                    );
                                }
                            }
                        }
                    }
                }

                if (!strong->isRtmp) {
                    for (auto it = strong->sharedVideoState.begin(); it != strong->sharedVideoState.end();) {
                        if (!usedEndpoints.contains(it->first) && !strong->videoChannels.contains(it->first)) {
                            it = strong->sharedVideoState.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        },
        [weak] () -> MediaSegment* {
            const auto strong = weak.lock();
            if (!strong) {
                return nullptr;
            }
            std::lock_guard lock(strong->segmentMutex);
            if (strong->waitForBufferedMillisecondsBeforeRendering) {
                if (strong->getAvailableBufferDuration() < strong->waitForBufferedMillisecondsBeforeRendering.value()) {
                    return nullptr;
                }
                strong->waitForBufferedMillisecondsBeforeRendering = std::nullopt;
            }
            const auto availableSegments = strong->filterSegments(MediaSegment::Status::Ready);
            if (availableSegments.empty()) {
                strong->waitForBufferedMillisecondsBeforeRendering = strong->segmentBufferDuration + strong->segmentDuration;
                return nullptr;
            }
            return availableSegments.begin()->second;
        },
        [weak] (const ThreadBuffer::RequestType requestType) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            std::lock_guard lock(strong->segmentMutex);
            switch (requestType) {
            case ThreadBuffer::RequestType::RequestSegments:
                strong->requestSegmentsIfNeeded();
                strong->checkPendingSegments();
                break;
            case ThreadBuffer::RequestType::RemoveSegment:
                const auto segment = strong->segments.begin();
                if (segment == strong->segments.end()) {
                    return;
                }
                strong->segments.erase(segment);
                break;
            }
        });
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
                        sendBroadcastTimestamp(serverTimeMs + (webrtc::TimeMillis() - serverTimeMsGotAt));
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

            std::unique_ptr<MediaSegment::Part> audioPart;
            if (isRtmp) {
                audioPart = std::make_unique<MediaSegment::Part>(MediaSegment::Part::Unified());
            } else {
                audioPart = std::make_unique<MediaSegment::Part>(MediaSegment::Part::Audio());
            }
            pendingSegment->parts.push_back(std::move(audioPart));

            for (const auto &[endpoint, videoChannel] : videoChannels) {
                if (!currentEndpointMapping.contains(endpoint)) {
                    continue;
                }
                const int32_t channelId = currentEndpointMapping[endpoint] + 1;
                auto videoPart = std::make_unique<MediaSegment::Part>(MediaSegment::Part::Video(channelId, videoChannel.quality));
                pendingSegment->parts.push_back(std::move(videoPart));
            }
            if (segments.contains(nextSegmentTimestamp)) {
                return;
            }
            segments[nextSegmentTimestamp] = std::move(pendingSegment);

            if (nextSegmentTimestamp == -1) {
                break;
            }
        }
    }

    void MTProtoStream::checkPendingSegments() {
        if (!running) {
            return;
        }
        const auto absoluteTimestamp = webrtc::TimeMillis();
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
                    } else if (std::get_if<MediaSegment::Part::Unified>(typeData)) {
                        videoQuality = MediaSegment::Quality::Full;
                        videoChannelId = 1;
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
                            RTC_LOG(LS_VERBOSE) << "Video part " << pendingSegment->timestamp << " is empty";
                        }
                        videoSegment->part = std::make_unique<VideoStreamingPart>(std::move(part->data.value()));
                        pendingSegment->video.push_back(std::move(videoSegment));
                    } else if (std::get_if<MediaSegment::Part::Unified>(typeData)) {
                        auto unifiedSegment = std::make_unique<MediaSegment::Video>();
                        bytes::binary dataCopy = part->data.value();
                        unifiedSegment->part = std::make_unique<VideoStreamingPart>(std::move(part->data.value()));
                        pendingSegment->video.push_back(std::move(unifiedSegment));
                        pendingSegment->unifiedAudio = std::make_unique<VideoStreamingPart>(std::move(dataCopy), webrtc::MediaType::AUDIO);
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
                std::lock_guard lock(strong->segmentMutex);
                strong->checkPendingSegments();
            }, webrtc::TimeDelta::Millis(std::max(static_cast<int32_t>(minDelayedRequestTimeout), 10)));
        }

        if (shouldRequestMoreSegments) {
            requestSegmentsIfNeeded();
        }
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