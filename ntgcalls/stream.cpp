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
            if (lipSync) {
                if (audio->time() < video->time()) {
                    bs = audio;
                    br = reader->audio;
                } else {
                    bs = video;
                    br = reader->video;
                }
            } else {
                // TODO: LipSyncless Implementation
            }
        } else if (reader->audio) {
            bs = audio;
            br = reader->audio;
        } else {
            bs = video;
            br = reader->video;
        }

        auto waitTime = bs->waitTime();
        if (waitTime > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(waitTime));
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
            auto bs = bsBR.first;
            auto br = bsBR.second;
            bs->sendData(br->read(bs->frameSize()));

            checkStream();
        }

        if (running) {
            dispatchQueue.dispatch([this]() {
                this->sendSample();
            });
        }
    }

    void Stream::setAVStream(MediaDescription streamConfig) {
        auto audioConfig = streamConfig.audio;
        auto videoConfig = streamConfig.video;
        reader = std::make_shared<MediaReaderFactory>(streamConfig);
        lipSync = audioConfig && videoConfig && audioConfig->path == videoConfig->path;

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

    void Stream::start() {
        if (!running) {
            running = true;
            dispatchQueue.dispatch([this]() {
                sendSample();
            });
        }
    }

    void Stream::pause() {
        idling = true;
    }

    void Stream::resume() {
        idling = false;
    }

    void Stream::mute() {
        audioTrack->Mute(true);
        videoTrack->Mute(true);
    }

    void Stream::unmute() {
        audioTrack->Mute(false);
        videoTrack->Mute(false);
    }

    void Stream::stop() {
        running = false;
    }

    void Stream::onStreamEnd(std::function<void(Stream::Type)> &callback) {
        onEOF = callback;
    }
}