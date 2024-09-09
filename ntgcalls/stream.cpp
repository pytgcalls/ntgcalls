//
// Created by Laky64 on 12/08/2023.
//

#include "stream.hpp"

namespace ntgcalls {
    Stream::Stream(rtc::Thread* workerThread): workerThread(workerThread) {
        audio = std::make_unique<AudioStreamer>();
        video = std::make_unique<VideoStreamer>();
    }

    Stream::~Stream() {
        RTC_LOG(LS_VERBOSE) << "Destroying Stream";
        std::unique_lock lock(mutex);
        onEOF = nullptr;
        lock.unlock();
        quit = true;
        if (thread.joinable()) {
            thread.join();
        }
        RTC_LOG(LS_VERBOSE) << "Thread joined";
        lock.lock();
        idling = false;
        audio = nullptr;
        video = nullptr;
        audioTrack = nullptr;
        videoTrack = nullptr;
        reader = nullptr;
        workerThread = nullptr;
        RTC_LOG(LS_VERBOSE) << "Stream destroyed";
    }

    void Stream::addTracks(const std::unique_ptr<wrtc::NetworkInterface>& pc) {
        audioTrack = pc->addTrack(audio->createTrack());
        videoTrack = pc->addTrack(video->createTrack());
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
        RTC_LOG(LS_INFO) << "Setting AVStream, Acquiring lock";
        changing = true;
        std::lock_guard lock(mutex);
        RTC_LOG(LS_INFO) << "Setting AVStream, Lock acquired";
        const auto audioConfig = streamConfig.audio;
        const auto videoConfig = streamConfig.video;
        const bool wasIdling = idling;
        idling = false;
        if (audioConfig) {
            audio->setConfig(
                audioConfig->sampleRate,
                audioConfig->bitsPerSample,
                audioConfig->channelCount
            );
            RTC_LOG(LS_INFO) << "Audio config set";
        }
        const bool wasVideo = hasVideo;
        if (videoConfig) {
            hasVideo = true;
            video->setConfig(
                videoConfig->width,
                videoConfig->height,
                videoConfig->fps
            );
            RTC_LOG(LS_INFO) << "Video config set";
        } else {
            hasVideo = false;
        }
        RTC_LOG(LS_INFO) << "Creating MediaReaderFactory";
        reader = std::make_unique<MediaReaderFactory>(streamConfig, audio->frameSize(), video->frameSize());
        RTC_LOG(LS_INFO) << "MediaReaderFactory created";
        if ((wasVideo != hasVideo || wasIdling) && !noUpgrade) {
            checkUpgrade();
        }
        changing = false;
    }

    void Stream::checkUpgrade() {
        workerThread->PostTask([&] {
            (void) onChangeStatus(getState());
        });
    }

    MediaState Stream::getState() {
        std::shared_lock lock(mutex);
        return MediaState{
            (audioTrack ? !audioTrack->enabled() : false) && (videoTrack ? !videoTrack->enabled() : false),
            idling || (videoTrack ? !videoTrack->enabled() : false),
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
                if (idling || changing || !reader || !(reader->audio || reader->video)) {
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    lock.lock();
                } else {
                    BaseStreamer* bs;
                    BaseReader* br;
                    if (reader->audio && reader->video) {
                        if (audio->nanoTime() <= video->nanoTime()) {
                            bs = audio.get();
                            br = reader->audio.get();
                        } else {
                            bs = video.get();
                            br = reader->video.get();
                        }
                    } else if (reader->audio) {
                        bs = audio.get();
                        br = reader->audio.get();
                    } else {
                        bs = video.get();
                        br = reader->video.get();
                    }
                    if (auto [sample, captureTime] = br->read(); sample && bs) {
                        if (const auto waitTime = bs->waitTime(); std::chrono::duration_cast<std::chrono::milliseconds>(waitTime).count() > 0) {
                            lock.unlock();
                            std::this_thread::sleep_for(waitTime);
                            lock.lock();
                        }
                        bs->sendData(sample.get(), captureTime);
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
        if (audioTrack && !audioTrack->enabled() != isMuted) {
            audioTrack->set_enabled(!isMuted);
            changed = true;
        }
        if (videoTrack && !videoTrack->enabled() != isMuted) {
            videoTrack->set_enabled(!isMuted);
            changed = true;
        }
        if (changed) {
            checkUpgrade();
        }
        return changed;
    }

    void Stream::onStreamEnd(const std::function<void(Type)> &callback) {
        onEOF = callback;
    }

    void Stream::onUpgrade(const std::function<void(MediaState)> &callback) {
        onChangeStatus = callback;
    }
}
