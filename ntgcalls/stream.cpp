//
// Created by Laky64 on 12/08/2023.
//

#include "stream.hpp"

namespace ntgcalls {
    Stream::Stream() {
        audio = std::make_shared<AudioStreamer>();
        video = std::make_shared<VideoStreamer>();
    }

    Stream::~Stream() {
        stop();
        audio = nullptr;
        video = nullptr;
        audioTrack = nullptr;
        videoTrack = nullptr;
        reader = nullptr;
        running = false;
    }

    void Stream::addTracks(const std::shared_ptr<wrtc::PeerConnection>& pc) {
        audioTrack = audio->createTrack();
        videoTrack = video->createTrack();
        pc->addTrack(audioTrack);
        pc->addTrack(videoTrack);
    }

    std::pair<std::shared_ptr<BaseStreamer>, std::shared_ptr<BaseReader>> Stream::unsafePrepareForSample() {
        std::shared_ptr<BaseStreamer> bs;
        std::shared_ptr<BaseReader> br;
        if (reader->audio && reader->video) {
            if (audio->nanoTime() <= video->nanoTime()) {
                bs = audio;
                br = reader->audio;
            } else {
                bs = video;
                br = reader->video;
            }
        } else if (reader->audio) {
            bs = audio;
            br = reader->audio;
        } else {
            bs = video;
            br = reader->video;
        }

        auto waitTime = bs->waitTime();
        if (waitTime.count() > 0) {
            std::this_thread::sleep_for(waitTime);
        }
        return {bs, br};
    }

    void Stream::checkStream() {
        if (reader->audio && reader->audio->eof()) {
            reader->audio = nullptr;
            onEOF(Audio);
        }
        if (reader->video && reader->video->eof()) {
            reader->video = nullptr;
            onEOF(Video);
        }
    }

    void Stream::sendSample() {
        if (idling || !(reader->audio || reader->video)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } else {
            auto bsBR = unsafePrepareForSample();
            bsBR.first->sendData(bsBR.second->read(bsBR.first->frameSize()));
            checkStream();
        }

        if (running) {
            this->sendSample();
        }
    }

    void Stream::setAVStream(MediaDescription streamConfig) {
        auto audioConfig = streamConfig.audio;
        auto videoConfig = streamConfig.video;
        reader = std::make_shared<MediaReaderFactory>(streamConfig);
        idling = false;
        if (audioConfig) {
            audio->setConfig(
                audioConfig->sampleRate,
                audioConfig->bitsPerSample,
                audioConfig->channelCount
            );
        }
        if (videoConfig) {
            video->setConfig(
                videoConfig->width,
                videoConfig->height,
                videoConfig->fps
            );
        }
    }

    uint64_t Stream::time() {
        if (reader->audio && reader->video) {
            return (audio->time() + video->time()) / 2;
        } else if (reader->audio) {
            return audio->time();
        } else if (reader->video) {
            return video->time();
        }
        return 0;
    }

    void Stream::start() {
        if (!running) {
            running = true;
            dispatchQueue.dispatch([this]() {
                sendSample();
            });
        }
    }

    bool Stream::pause() {
        return !std::exchange(idling, true);
    }

    bool Stream::resume() {
        return std::exchange(idling, false);
    }

    bool Stream::mute() {
        if (!audioTrack->isMuted() || !videoTrack->isMuted()) {
            audioTrack->Mute(true);
            videoTrack->Mute(true);
            return true;
        } else {
            return false;
        }
    }

    bool Stream::unmute() {
        if (audioTrack->isMuted() || videoTrack->isMuted()) {
            audioTrack->Mute(false);
            videoTrack->Mute(false);
            return true;
        } else {
            return false;
        }
    }

    void Stream::stop() {
        running = false;
        idling = false;
        if (reader->audio) {
            reader->audio->close();
        }
        if (reader->video) {
            reader->video->close();
        }
        dispatchQueue.removePending();
    }

    void Stream::onStreamEnd(std::function<void(Stream::Type)> &callback) {
        onEOF = callback;
    }
}