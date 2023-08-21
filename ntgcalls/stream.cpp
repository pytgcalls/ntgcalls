//
// Created by Laky64 on 12/08/2023.
//

#include "stream.hpp"
#include "utils/time.hpp"

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
        rAudio = nullptr;
        rVideo = nullptr;
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
        if (rAudio && rVideo) {
            if (lipSync) {
                if (audio->time() < video->time()) {
                    bs = audio;
                    br = rAudio;
                } else {
                    bs = video;
                    br = rVideo;
                }
            } else {
                // TODO: LipSyncless Implementation
            }
        } else if (rAudio && !rVideo) {
            bs = audio;
            br = rAudio;
        } else {
            bs = video;
            br = rVideo;
        }

        auto waitTime = bs->waitTime();
        if (waitTime > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(waitTime));
        }
        return {bs, br};
    }

    void Stream::checkStream() {
        if (rAudio && rAudio->eof()) {
            rAudio = nullptr;
            onEOF(Audio);
        }
        if (rVideo && rVideo->eof()) {
            rVideo = nullptr;
            onEOF(Video);
        }
    }

    void Stream::sendSample() {
        if (idling || !(rAudio || rVideo)) {
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

    void Stream::setAVStream(StreamConfig streamConfig) {
        auto audioConfig = streamConfig.audioConfig;
        auto videoConfig = streamConfig.videoConfig;

        lipSync = streamConfig.lipSync && audioConfig && videoConfig;

        if (audioConfig) {
            audio->setConfig(
                audioConfig->sampleRate,
                audioConfig->bitsPerSample,
                audioConfig->channelCount
            );
            rAudio = audioConfig->reader;
        } else {
            rAudio = nullptr;
        }
        if (videoConfig) {
            video->setConfig(
                videoConfig->width,
                videoConfig->height,
                videoConfig->fps
            );
            rVideo = videoConfig->reader;
        } else {
            rVideo = nullptr;
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