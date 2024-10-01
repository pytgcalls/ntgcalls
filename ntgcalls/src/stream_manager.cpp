//
// Created by Laky64 on 28/09/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/stream_manager.hpp>
#include <ntgcalls/media/audio_sink.hpp>
#include <ntgcalls/media/audio_streamer.hpp>
#include <ntgcalls/media/media_reader_factory.hpp>
#include <ntgcalls/media/video_sink.hpp>
#include <ntgcalls/media/video_streamer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls {

    StreamManager::StreamManager(rtc::Thread* workerThread): workerThread(workerThread) {}

    StreamManager::~StreamManager() {
        RTC_LOG(LS_VERBOSE) << "Destroying Stream";
        onEOF = nullptr;
        readers.clear();
        streams.clear();
        tracks.clear();
        workerThread = nullptr;
        RTC_LOG(LS_VERBOSE) << "Stream destroyed";
    }

    void StreamManager::enableVideoSimulcast(const bool enable) {
       videoSimulcast = enable;
    }

    void StreamManager::setStreamSources(const Mode mode, const MediaDescription& desc) {
        RTC_LOG(LS_INFO) << "Setting Configuration, Acquiring lock";
        std::lock_guard lock(mutex);
        RTC_LOG(LS_INFO) << "Setting Configuration, Lock acquired";

        const bool wasIdling = isPaused();

        setConfig<AudioSink, AudioDescription>(mode, Microphone, desc.microphone);
        setConfig<AudioSink, AudioDescription>(mode, Speaker, desc.speaker);

        const bool wasCamera = hasDevice(mode, Camera);
        const bool wasScreen = hasDevice(mode, Screen);

        if (!videoSimulcast && desc.camera && desc.screen) {
            throw InvalidParams("Cannot mix camera and screen sources");
        }

        setConfig<VideoSink, VideoDescription>(mode, Camera, desc.camera);
        setConfig<VideoSink, VideoDescription>(mode, Screen, desc.screen);

        if (mode == Playback && (wasCamera != hasDevice(mode, Camera) || wasScreen != hasDevice(mode, Screen) || wasIdling) && initialized) {
            checkUpgrade();
        }

        if (!initialized && mode == Playback) {
            initialized = true;
        }
    }

    MediaState StreamManager::getState() {
        std::shared_lock lock(mutex);
        bool muted = false;
        for (const auto& [key, track] : tracks) {
            if (key.first != Playback) {
                continue;
            }
            if (!track->enabled()) {
                muted = true;
                break;
            }
        }
        const auto paused = isPaused();
        return MediaState{
            muted,
            (paused || muted),
            !hasDevice(Playback, Camera) && !hasDevice(Playback, Screen),
            (paused || muted),
        };
    }

    bool StreamManager::pause() {
        return updatePause(true);
    }

    bool StreamManager::resume() {
        return updatePause(false);
    }

    bool StreamManager::mute() {
        return updateMute(true);
    }

    bool StreamManager::unmute() {
        return updateMute(false);
    }

    uint64_t StreamManager::time(const Mode mode) {
        std::shared_lock lock(mutex);
        uint64_t averageTime = 0;
        for (const auto& [key, stream] : streams) {
            if (stream->time() == 0 || key.first != mode) {
                continue;
            }
            averageTime += stream->time();
        }
        return averageTime / streams.size();
    }

    StreamManager::Status StreamManager::status(const Mode mode) {
        std::shared_lock lock(mutex);
        if (mode == Playback) {
            return readers.empty() ? Idling : isPaused() ? Paused : Active;
        }
        // TODO: Implement input status
        return Idling;
    }

    void StreamManager::onStreamEnd(const std::function<void(Type, Device)>& callback) {
        onEOF = callback;
    }

    void StreamManager::onUpgrade(const std::function<void(MediaState)>& callback) {
        onChangeStatus = callback;
    }

    void StreamManager::addTrack(Mode mode, Device device, const std::unique_ptr<wrtc::NetworkInterface>& pc) {
        const std::pair id(mode, device);
        tracks[id] = pc->addTrack(streams[id]->createTrack());
    }

    void StreamManager::start() {
        // ReSharper disable once CppUseElementsView
        for (const auto& [key, reader] : readers) {
            reader->open();
        }
    }

    bool StreamManager::hasDevice(const Mode mode, const Device device) const {
        if (mode == Playback) {
            return readers.contains(device);
        }
        return false;
    }

    bool StreamManager::updateMute(const bool isMuted) {
        std::lock_guard lock(mutex);
        bool changed = false;
        for (const auto& [key, track] : tracks) {
            if (key.first != Playback) {
                continue;
            }
            if (!track->enabled() != isMuted) {
                track->set_enabled(!isMuted);
                changed = true;
            }
        }
        if (changed) {
            checkUpgrade();
        }
        return changed;
    }

    bool StreamManager::updatePause(const bool isPaused) {
        std::lock_guard lock(mutex);
        auto res = false;
        // ReSharper disable once CppUseElementsView
        for (const auto& [key, reader] : readers) {
            if (reader->set_enabled(!isPaused)) {
                res = true;
            }
        }
        if (res) {
            checkUpgrade();
        }
        return res;
    }

    bool StreamManager::isPaused() {
        auto res = false;
        // ReSharper disable once CppUseElementsView
        for (const auto& [key, reader] : readers) {
            if (!reader->is_enabled()) {
                res = true;
            }
        }
        return res;
    }

    StreamManager::Type StreamManager::getStreamType(const Device device) {
        switch (device) {
        case Microphone:
        case Speaker:
            return Audio;
        case Camera:
        case Screen:
            return Video;
        default:
            RTC_LOG(LS_ERROR) << "Invalid device kind";
            throw InvalidParams("Invalid device kind");
        }
    }

    void StreamManager::checkUpgrade() {
        workerThread->PostTask([&] {
            (void) onChangeStatus(getState());
        });
    }

    template <typename SinkType, typename DescriptionType>
    void StreamManager::setConfig(Mode mode, Device device, const std::optional<DescriptionType>& desc) {
        std::pair id(mode, device);
        if (!videoSimulcast && (device == Camera || device == Screen)) {
            id = std::make_pair(mode, Camera);
        }
        const auto streamType = getStreamType(device);

        if (!streams.contains(id)) {
            if (mode == Playback) {
                if (streamType == Audio) {
                    streams[id] = std::make_unique<AudioStreamer>();
                } else {
                    streams[id] = std::make_unique<VideoStreamer>();
                }
            } else {
                throw InvalidParams("Capture streams are not yet supported");
            }
        }

        if (desc) {
            auto sink = dynamic_cast<SinkType*>(streams[id].get());
            if (sink && sink->setConfig(desc)) {
                if (mode == Playback) {
                    readers[device] = MediaReaderFactory::fromInput(desc.value(), streams[id].get());
                    readers[device]->onData([this, id](const bytes::unique_binary& data) {
                        dynamic_cast<BaseStreamer*>(streams[id].get())->sendData(data.get(), rtc::TimeMillis());
                    });
                    readers[device]->onEof([this, device] {
                        workerThread->PostTask([this, device] {
                            (void) onEOF(getStreamType(device), device);
                            std::lock_guard lock(mutex);
                            readers.erase(device);
                        });
                    });
                } else {
                    throw InvalidParams("Capture streams are not yet supported");
                }
            }
        } else if (mode == Playback) {
            readers.erase(device);
        }
    }
} // ntgcalls