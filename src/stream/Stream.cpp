#include "Stream.hpp"

Stream::Stream(const std::shared_ptr<StreamSource>& audio, const std::shared_ptr<StreamSource>& video) {
    if (audio != nullptr) audio->init();
    if (video != nullptr) video->init();
    audioSrc = audio;
    videoSrc = video;
}

Stream::~Stream() {
    stop();
}

void Stream::setState(State s) {
    std::unique_lock lock(mutexState);
    state = s;
}

Stream::State Stream::getState() {
    std::shared_lock lock(mutexState);
    return state;
}

void Stream::addTracks(const std::shared_ptr<rtc::PeerConnection>& pc) {
    if (audioSrc != nullptr) {
        addTrack(pc, audioSrc);
    }

    if (videoSrc != nullptr) {
        addTrack(pc, videoSrc);
    }
}

void Stream::addTrack(const std::shared_ptr<rtc::PeerConnection>& pc,
                      const std::shared_ptr<StreamSource>& mediaSrc) {
    mediaSrc->track = pc->addTrack(mediaSrc->desc->getMedia());
    mediaSrc->track->setMediaHandler(mediaSrc->mediaHandler);
    mediaSrc->track->onOpen([this, mediaSrc]() {preStart(mediaSrc->isVideo);});
}

void Stream::preStart(bool isVideo) {
    if (getState() == State::Waiting) {
        if (audioSrc == nullptr || videoSrc == nullptr) {
            setState(State::Ready);
        } else {
            setState(isVideo ? State::WaitingForAudio : State::WaitingForVideo);
        }
    } else if ((getState() == State::WaitingForAudio && !isVideo) || (getState() == State::WaitingForVideo && isVideo)) {
        setState(State::Ready);
    }
    if (getState() == State::Ready) {
        sendInitialNalus();
        start();
    }
}

void Stream::start() const {
    std::lock_guard lock(mutex);
    if (isRunning) {
        return;
    }
    isRunning = true;
    startTime = getMicroseconds();
    if (audioSrc != nullptr) audioSrc->start();
    if (videoSrc != nullptr) videoSrc->start();
    dispatchQueue.dispatch([this]() {
        this->sendSample();
    });
}

void Stream::stop() {
    std::lock_guard lock(mutex);
    if (!isRunning) {
        return;
    }
    isRunning = false;
    dispatchQueue.removePending();
    if (audioSrc != nullptr) audioSrc->stop();
    if (videoSrc != nullptr) videoSrc->stop();
}

void Stream::sendSample() const {
    std::lock_guard lock(mutex);
    if (!isRunning) {
        return;
    }
    auto streamSource = unsafePrepareForSample();
    auto sample = streamSource->getSample();
    sampleHandler(streamSource, sample);
    streamSource->loadNextSample();
    dispatchQueue.dispatch([this]() {
        this->sendSample();
    });
}



std::shared_ptr<StreamSource> Stream::unsafePrepareForSample() const {
    std::shared_ptr<StreamSource> ss;
    uint64_t nextTime;
    if (videoSrc == nullptr || audioSrc->getSampleTime_us() < videoSrc->getSampleTime_us()) {
        ss = audioSrc;
        nextTime = audioSrc->getSampleTime_us();
    } else {
        ss = videoSrc;
        nextTime = videoSrc->getSampleTime_us();
    }

    auto currentTime = getMicroseconds();

    auto elapsed = currentTime - startTime;
    if (nextTime > elapsed) {
        auto waitTime = nextTime - elapsed;
        mutex.unlock();
        usleep(waitTime);
        mutex.lock();
    }
    return ss;
}

void Stream::sampleHandler(const std::shared_ptr<StreamSource>& streamSource, const rtc::binary& sample) {
    uint64_t sampleTime = streamSource->getSampleTime_us();
    auto rtpConfig = streamSource->srReporter->rtpConfig;
    auto elapsedSeconds = double(sampleTime) / (1000 * 1000);
    uint32_t elapsedTimestamp = rtpConfig->secondsToTimestamp(elapsedSeconds);
    rtpConfig->timestamp = rtpConfig->startTimestamp + elapsedTimestamp;
    auto reportElapsedTimestamp = rtpConfig->timestamp - streamSource->srReporter->lastReportedTimestamp();
    if (rtpConfig->timestampToSeconds(reportElapsedTimestamp) > 1) {
        streamSource->srReporter->setNeedsToReport();
    }
    streamSource->track->send(sample);
}

void Stream::sendInitialNalus() {
    /*auto h264 = dynamic_cast<H264Reader *>(videoSrc.get());
    auto initialNalus = h264->initialNALUS();

    if (!initialNalus.empty()) {
        auto rtpConfig = videoSrc->srReporter->rtpConfig;
        const double frameDuration_s = double(h264->getSampleDuration_us()) / (1000 * 1000);
        const uint32_t frameTimestampDuration = rtpConfig->secondsToTimestamp(frameDuration_s);
        rtpConfig->timestamp = rtpConfig->startTimestamp - frameTimestampDuration * 2;
        videoSrc->track->send(initialNalus);
        rtpConfig->timestamp += frameTimestampDuration;
        videoSrc->track->send(initialNalus);
    }*/
}
