//
// Created by Laky64 on 12/08/2023.
//

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
        std::lock_guard lock(mutex);
        audio = nullptr;
        video = nullptr;
        audioTrack = nullptr;
        videoTrack = nullptr;
        reader = nullptr;
        updateQueue = nullptr;
    }

    void Stream::addTracks(const std::shared_ptr<wrtc::PeerConnection>& pc) {
        pc->addTrack(audioTrack = audio->createTrack());
        pc->addTrack(videoTrack = video->createTrack());
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
        if (const auto waitTime = bs->waitTime(); std::chrono::duration_cast<std::chrono::milliseconds>(waitTime).count() > 0) {
            mutex.unlock();
            std::this_thread::sleep_for(waitTime);
            mutex.lock();
        }
        return {bs, br};
    }

    void Stream::checkStream() const {
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

    void Stream::sendSample() {
        mutex.lock();
        if (running) {
            if (idling || !reader || !(reader->audio || reader->video)) {
                mutex.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                mutex.lock();
            } else {
                if (auto [fst, snd] = unsafePrepareForSample(); fst && snd) {
                    if (const auto sample = snd->read(fst->frameSize())) {
                        fst->sendData(sample);
                    }
                }
                checkStream();
            }
            if (streamQueue) {
                streamQueue->dispatch([&] {
                    sendSample();
                });
            }
        }
        mutex.unlock();
    }

    void Stream::setAVStream(const MediaDescription& streamConfig, const bool noUpgrade) {
        std::lock_guard lock(mutex);
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
        if (wasVideo != hasVideo && !noUpgrade) {
            checkUpgrade();
        }
    }

    void Stream::checkUpgrade() {
        updateQueue->dispatch([&] {
            (void) onChangeStatus(getState());
        });
    }

    MediaState Stream::getState() {
        std::lock_guard lock(mutex);
        return MediaState{
            (audioTrack ? audioTrack->isMuted() : false) && (videoTrack ? videoTrack->isMuted() : false),
            idling || (videoTrack ? videoTrack->isMuted() : false),
            !hasVideo
        };
    }

    uint64_t Stream::time() {
        std::lock_guard lock(mutex);
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

    Stream::Status Stream::status() {
        std::lock_guard lock(mutex);
        if (reader && (reader->audio || reader->video) && running) {
            return idling ? Paused : Playing;
        }
        return Idling;
    }

    void Stream::start() {
        std::lock_guard lock(mutex);
        if (!running) {
            running = true;
            streamQueue->dispatch([&] {
                sendSample();
            });
        }
    }

    bool Stream::pause() {
        std::lock_guard lock(mutex);
        const auto res = std::exchange(idling, true);
        checkUpgrade();
        return !res;
    }

    bool Stream::resume() {
        std::lock_guard lock(mutex);
        const auto res = std::exchange(idling, false);
        checkUpgrade();
        return res;
    }

    bool Stream::mute() {
        return updateMute(true);
    }

    bool Stream::unmute() {
        return updateMute(false);
    }

    bool Stream::updateMute(const bool isMuted) {
        std::lock_guard lock(mutex);
        bool changed = false;
        if (audioTrack && audioTrack->isMuted() != isMuted) {
            audioTrack->Mute(isMuted);
            changed = true;
        }
        if (videoTrack && videoTrack->isMuted() != isMuted) {
            videoTrack->Mute(isMuted);
            changed = true;
        }
        if (changed) {
            checkUpgrade();
        }
        return changed;
    }

    void Stream::stop() {
        running = false;
        idling = false;
        mutex.lock();
        if (reader) {
            if (reader->audio) {
                reader->audio->close();
            }
            if (reader->video) {
                reader->video->close();
            }
        }
        mutex.unlock();
        streamQueue = nullptr;
    }

    void Stream::onStreamEnd(const std::function<void(Type)> &callback) {
        onEOF = callback;
    }

    void Stream::onUpgrade(const std::function<void(MediaState)> &callback) {
        onChangeStatus = callback;
    }
}
