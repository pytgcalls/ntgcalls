//
// Created by Laky64 on 12/08/2023.
//

#include "stream.hpp"

namespace ntgcalls {
    Stream::Stream() {
        audio = std::make_unique<AudioStreamer>();
        video = std::make_unique<VideoStreamer>();
        workerThread = rtc::Thread::Create();
        workerThread->Start();
    }

    Stream::~Stream() {
        stop();
        workerThread->Stop();

        std::lock_guard lock(mutex);
        audio = nullptr;
        video = nullptr;
        audioTrack = nullptr;
        videoTrack = nullptr;
        reader = nullptr;
    }

    void Stream::addTracks(const std::unique_ptr<wrtc::PeerConnection>& pc) {
        pc->addTrack(audioTrack = audio->createTrack());
        pc->addTrack(videoTrack = video->createTrack());
    }

    void Stream::checkStream() const {
        if (reader->audio && reader->audio->eof()) {
            reader->audio = nullptr;
            workerThread->PostTask([&] {
                (void) onEOF(Audio);
            });
        }
        if (reader->video && reader->video->eof()) {
            reader->video = nullptr;
            workerThread->PostTask([&] {
                (void) onEOF(Video);
            });
        }
    }

    void Stream::setAVStream(const MediaDescription& streamConfig, const bool noUpgrade) {
        std::lock_guard lock(mutex);
        const auto audioConfig = streamConfig.audio;
        const auto videoConfig = streamConfig.video;
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
        reader = std::make_unique<MediaReaderFactory>(streamConfig, audio->frameSize(), video->frameSize());
        if (wasVideo != hasVideo && !noUpgrade) {
            checkUpgrade();
        }
    }

    void Stream::checkUpgrade() {
        workerThread->PostTask([&] {
            (void) onChangeStatus(getState());
        });
    }

    MediaState Stream::getState() {
        std::shared_lock lock(mutex);
        return MediaState{
            (audioTrack ? audioTrack->isMuted() : false) && (videoTrack ? videoTrack->isMuted() : false),
            idling || (videoTrack ? videoTrack->isMuted() : false),
            !hasVideo
        };
    }

    uint64_t Stream::time() {
        std::shared_lock lock(mutex);
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
        std::shared_lock lock(mutex);
        if (reader && (reader->audio || reader->video)) {
            return idling ? Paused : Playing;
        }
        return Idling;
    }

    void Stream::start() {
        thread = std::thread([this] {
            do {
                std::shared_lock lock(mutex);
                if (idling || !reader || !(reader->audio || reader->video)) {
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    lock.lock();
                } else {
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
                    if (auto [sample, captureTime] = br->read(); sample && bs) {
                        if (const auto waitTime = bs->waitTime(); std::chrono::duration_cast<std::chrono::milliseconds>(waitTime).count() > 0) {
                            lock.unlock();
                            std::this_thread::sleep_for(waitTime);
                            lock.lock();
                        }
                        bs->sendData(sample, captureTime);
                    }
                    checkStream();
                }
            } while (!quit);
        });
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
        quit = true;
        if (thread.joinable()) {
            thread.join();
        }
        std::lock_guard lock(mutex);
        idling = false;
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
