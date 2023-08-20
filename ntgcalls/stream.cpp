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
        is_audio = nullptr;
        is_video = nullptr;
        running = false;
    }

    void Stream::addTracks(const std::shared_ptr<wrtc::PeerConnection>& pc) {
        pc->addTrack(audio->createTrack());
        pc->addTrack(video->createTrack());
    }

    std::pair<std::shared_ptr<BaseStreamer>, std::shared_ptr<BaseReader>> Stream::unsafePrepareForSample() {
        std::shared_ptr<BaseStreamer> bs;
        std::shared_ptr<BaseReader> br;
        if (is_audio && is_video) {
            if (lipSync) {
                if (audio->time() < video->time()) {
                    bs = audio;
                    br = is_audio;
                } else {
                    bs = video;
                    br = is_video;
                }
            } else {
                // TODO: LipSyncless Implementation
            }
        } else if (is_audio && !is_video) {
            bs = audio;
            br = is_audio;
        } else {
            bs = video;
            br = is_video;
        }

        auto waitTime = bs->waitTime();
        if (waitTime > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(waitTime));
        }
        return {bs, br};
    }

    void Stream::sendSample() {
        if (idling || !(is_audio || is_video)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } else {
            auto bsBR = unsafePrepareForSample();
            auto bs = bsBR.first;
            auto br = bsBR.second;
            bs->sendData(br->read(bs->frameSize()));
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

        lipSync = streamConfig.lipSync;

        if (audioConfig) {
            audio->setConfig(
                    audioConfig->sampleRate,
                    audioConfig->bitsPerSample,
                    audioConfig->channelCount
            );
            is_audio = audioConfig->reader;
        } else {
            is_audio = nullptr;
        }
        if (videoConfig) {
            video->setConfig(
                    videoConfig->width,
                    videoConfig->height,
                    videoConfig->fps
            );
            is_video = videoConfig->reader;
        } else {
            is_video = nullptr;
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

    void Stream::stop() {
        running = false;
    }
}