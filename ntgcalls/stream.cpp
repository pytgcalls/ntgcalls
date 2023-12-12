//
// Created by Laky64 on 12/08/2023.
//

// ReSharper disable CppDFAUnreachableFunctionCall
#include "stream.hpp"

namespace ntgcalls {
    Stream::Stream() {
        audio = std::make_shared<AudioStreamer>();
        video = std::make_shared<VideoStreamer>();
        streamQueue = std::make_shared<DispatchQueue>(5);
        updateQueue = std::make_shared<DispatchQueue>();
    }

    Stream::~Stream() {
        stop();
        audio = nullptr;
        video = nullptr;
        audioTrack = nullptr;
        videoTrack = nullptr;
        reader = nullptr;
        updateQueue = nullptr;
    }

    void Stream::addTracks(const std::shared_ptr<wrtc::PeerConnection>& pc) {
        audioTrack = audio->createTrack();
        videoTrack = video->createTrack();
        pc->addTrack(audioTrack);
        pc->addTrack(videoTrack);
    }

    std::pair<std::shared_ptr<BaseStreamer>, std::shared_ptr<BaseReader>> Stream::unsafePrepareForSample() const {
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

        if (const auto waitTime = bs->waitTime(); waitTime.count() > 0) {
            std::this_thread::sleep_for(waitTime);
        }
        return {bs, br};
    }

    void Stream::checkStream() const {
        if (running && !changing) {
            if (reader->audio && reader->audio->eof()) {
                reader->audio = nullptr;
                updateQueue->dispatch([&] {
                    (void) onEOF(Audio);
                });
            }
            if (reader->video && reader->video->eof()) {
                reader->video = nullptr;
                updateQueue->dispatch([&] {
                    (void) onEOF(Video);
                });
            }
        }
    }

    void Stream::sendSample() {
        std::lock_guard lock(mutex);
        if (running) {
            if (idling || changing || !reader || !(reader->audio || reader->video)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } else {
                if (auto [fst, snd] = unsafePrepareForSample(); fst && snd) {
                    if (const auto sample = snd->read(fst->frameSize())) {
                        fst->sendData(sample);
                    }
                }
                checkStream();
            }
            if (streamQueue) {
                streamQueue->dispatch([this] {
                    sendSample();
                });
            }
        }
    }

    void Stream::setAVStream(const MediaDescription& streamConfig, const bool noUpgrade) {
        changing = true;
        const auto audioConfig = streamConfig.audio;
        const auto videoConfig = streamConfig.video;
        reader = std::make_shared<MediaReaderFactory>(streamConfig);
        idling = false;
        if (audioConfig) {
            audio->setConfig(
                audioConfig->sampleRate,
                audioConfig->bitsPerSample,
                audioConfig->channelCount
            );
        }
        const bool wasVideo = hasVideo;
        if (videoConfig) {
            hasVideo = true;
            video->setConfig(
                videoConfig->width,
                videoConfig->height,
                videoConfig->fps
            );
        } else {
            hasVideo = false;
        }
        changing = false;
        if (wasVideo != hasVideo && !noUpgrade) {
            checkUpgrade();
        }
    }

    void Stream::checkUpgrade() const {
        updateQueue->dispatch([&] {
            (void) onChangeStatus(getState());
        });
    }

    MediaState Stream::getState() const {
        return MediaState{
            audioTrack->isMuted() && videoTrack->isMuted(),
            idling || videoTrack->isMuted(),
            !hasVideo
        };
    }

    uint64_t Stream::time() const {
        if (reader) {
            if (reader->audio && reader->video) {
                return (audio->time() + video->time()) / 2;
            }
            if (reader->audio) {
                return audio->time();
            }
            if (reader->video) {
                return video->time();
            }
        }
        return 0;
    }

    Stream::Status Stream::status() const {
        if (reader && (reader->audio || reader->video) && running && !changing) {
            return idling ? Paused : Playing;
        }
        return Idling;
    }

    void Stream::start() {
        if (!running) {
            running = true;
            streamQueue->dispatch([this] {
                sendSample();
            });
        }
    }

    bool Stream::pause() {
        const auto res = std::exchange(idling, true);
        checkUpgrade();
        return !res;
    }

    bool Stream::resume() {
        const auto res = std::exchange(idling, false);
        checkUpgrade();
        return res;
    }

    bool Stream::mute() const {
        if (!audioTrack->isMuted() || !videoTrack->isMuted()) {
            audioTrack->Mute(true);
            videoTrack->Mute(true);
            checkUpgrade();
            return true;
        }
        checkUpgrade();
        return false;
    }

    bool Stream::unmute() const {
        if (audioTrack->isMuted() || videoTrack->isMuted()) {
            audioTrack->Mute(false);
            videoTrack->Mute(false);
            checkUpgrade();
            return true;
        }
        checkUpgrade();
        return false;
    }

    void Stream::stop() {
        running = false;
        idling = false;
        changing = false;
        streamQueue = nullptr;
        if (reader) {
            if (reader->audio) {
                reader->audio->close();
            }
            if (reader->video) {
                reader->video->close();
            }
        }
    }

    void Stream::onStreamEnd(const std::function<void(Type)> &callback) {
        onEOF = callback;
    }

    void Stream::onUpgrade(const std::function<void(MediaState)> &callback) {
        onChangeStatus = callback;
    }
}