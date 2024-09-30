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
        idling = false;
        readers.clear();
        streams.clear();
        tracks.clear();
        workerThread = nullptr;
        RTC_LOG(LS_VERBOSE) << "Stream destroyed";
    }

    void StreamManager::setStreamSources(const Direction direction, const MediaDescription& desc) {
        RTC_LOG(LS_INFO) << "Setting Configuration, Acquiring lock";
        std::lock_guard lock(mutex);
        RTC_LOG(LS_INFO) << "Setting Configuration, Lock acquired";

        const bool wasIdling = idling;
        if (direction == Output) idling = false;

        setConfig<AudioSink, AudioDescription>(direction, Microphone, desc.microphone);
        setConfig<AudioSink, AudioDescription>(direction, Speaker, desc.speaker);

        const bool wasVideo = hasVideo;
        if (direction == Output) hasVideo = desc.camera || desc.screen;

        setConfig<VideoSink, VideoDescription>(direction, Camera, desc.camera);
        setConfig<VideoSink, VideoDescription>(direction, Screen, desc.screen);

        if (direction == Output && (wasVideo != hasVideo || wasIdling) && initialized) {
            checkUpgrade();
        }

        if (!initialized && direction == Output) {
            initialized = true;
        }
    }

    MediaState StreamManager::getState() {
        std::shared_lock lock(mutex);
        bool muted = false;
        for (const auto& [key, track] : tracks) {
            if (key.first != Output) {
                continue;
            }
            if (!track->enabled()) {
                muted = true;
                break;
            }
        }
        return MediaState{
            muted,
            idling || muted,
            !hasVideo
        };
    }

    bool StreamManager::pause() {
        std::lock_guard lock(mutex);
        const auto res = std::exchange(idling, true);
        checkUpgrade();
        return !res;
    }

    bool StreamManager::resume() {
        std::lock_guard lock(mutex);
        const auto res = std::exchange(idling, false);
        checkUpgrade();
        return res;
    }

    bool StreamManager::mute() {
        return updateMute(true);
    }

    bool StreamManager::unmute() {
        return updateMute(false);
    }

    uint64_t StreamManager::time(const Direction direction) {
        std::shared_lock lock(mutex);
        uint64_t averageTime = 0;
        for (const auto& [key, stream] : streams) {
            if (stream->time() == 0 || key.first != direction) {
                continue;
            }
            averageTime += stream->time();
        }
        return averageTime / streams.size();
    }

    StreamManager::Status StreamManager::status(const Direction direction) {
        std::shared_lock lock(mutex);
        if (direction == Output) {
            return readers.empty() ? Idling : idling ? Paused : Active;
        }
        // TODO: Implement input status
        return Idling;
    }

    void StreamManager::onStreamEnd(const std::function<void(Device)>& callback) {
        onEOF = callback;
    }

    void StreamManager::onUpgrade(const std::function<void(MediaState)>& callback) {
        onChangeStatus = callback;
    }

    void StreamManager::addTrack(Direction direction, Device device, const std::unique_ptr<wrtc::NetworkInterface>& pc) {
        const std::pair id(direction, device);
        tracks[id] = pc->addTrack(streams[id]->createTrack());
    }

    void StreamManager::start() {
        // ReSharper disable once CppUseElementsView
        for (const auto& [key, reader] : readers) {
            reader->open();
        }
    }

    bool StreamManager::updateMute(const bool isMuted) {
        std::lock_guard lock(mutex);
        bool changed = false;
        for (const auto& [key, track] : tracks) {
            if (key.first != Output) {
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
    void StreamManager::setConfig(Direction direction, Device device, const std::optional<DescriptionType>& desc) {
        const std::pair id(direction, device);
        const auto streamType = getStreamType(device);

        if (!streams.contains(id)) {
            if (direction == Output) {
                if (streamType == Audio) {
                    streams[id] = std::make_unique<AudioStreamer>();
                } else {
                    streams[id] = std::make_unique<VideoStreamer>();
                }
            } else {
                throw InvalidParams("Output streams are not yet supported");
            }
        }

        if (desc) {
            auto sink = dynamic_cast<SinkType*>(streams[id].get());
            if (sink && sink->setConfig(desc)) {
                if (direction == Output) {
                    readers[device] = MediaReaderFactory::fromInput(desc.value(), streams[id].get());
                    readers[device]->onData([this, id](const bytes::unique_binary& data) {
                        dynamic_cast<BaseStreamer*>(streams[id].get())->sendData(data.get(), rtc::TimeMillis());
                    });
                    readers[device]->onEof([this, id] {
                        (void) onEOF(id.second);
                        std::lock_guard lock(mutex);
                        readers.erase(id.second);
                    });
                } else {
                    throw InvalidParams("Output streams are not yet supported");
                }
            }
        } else if (direction == Output) {
            readers.erase(device);
        }
    }
} // ntgcalls