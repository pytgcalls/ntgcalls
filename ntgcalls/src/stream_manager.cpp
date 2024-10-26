//
// Created by Laky64 on 28/09/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/stream_manager.hpp>
#include <ntgcalls/media/audio_receiver.hpp>
#include <ntgcalls/media/audio_sink.hpp>
#include <ntgcalls/media/audio_streamer.hpp>
#include <ntgcalls/media/base_receiver.hpp>
#include <ntgcalls/media/media_source_factory.hpp>
#include <ntgcalls/media/video_sink.hpp>
#include <ntgcalls/media/video_streamer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls {

    StreamManager::StreamManager(rtc::Thread* workerThread): workerThread(workerThread) {}

    StreamManager::~StreamManager() {
        RTC_LOG(LS_VERBOSE) << "Destroying Stream";
        onEOF = nullptr;
        readers.clear();
        writers.clear();
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

        if (mode == Capture && (wasCamera != hasDevice(mode, Camera) || wasScreen != hasDevice(mode, Screen) || wasIdling) && initialized) {
            checkUpgrade();
        }

        if (!initialized && mode == Capture) {
            initialized = true;
        }
    }

    MediaState StreamManager::getState() {
        std::shared_lock lock(mutex);
        bool muted = false;
        for (const auto& [key, track] : tracks) {
            if (key.first != Capture) {
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
            !hasDevice(Capture, Camera) && !hasDevice(Capture, Screen),
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
        int count = 0;
        for (const auto& [key, stream] : streams) {
            if (stream->time() == 0 || key.first != mode) {
                continue;
            }
            averageTime += stream->time();
            count++;
        }
        return averageTime / count;
    }

    StreamManager::Status StreamManager::status(const Mode mode) {
        std::shared_lock lock(mutex);
        if (mode == Capture) {
            return readers.empty() ? Idling : isPaused() ? Paused : Active;
        }
        return writers.empty() ? Idling : Active;
    }

    void StreamManager::onStreamEnd(const std::function<void(Type, Device)>& callback) {
        onEOF = callback;
    }

    void StreamManager::onUpgrade(const std::function<void(MediaState)>& callback) {
        onChangeStatus = callback;
    }

    void StreamManager::addTrack(Mode mode, Device device, const std::unique_ptr<wrtc::NetworkInterface>& pc) {
        const std::pair id(mode, device);
        if (mode == Capture) {
            tracks[id] = pc->addOutgoingTrack(dynamic_cast<BaseStreamer*>(streams[id].get())->createTrack());
        } else {
            pc->addIncomingTrack(dynamic_cast<BaseReceiver*>(streams[id].get())->remoteSink());
        }
    }

    void StreamManager::start() {
        // ReSharper disable once CppUseElementsView
        for (const auto& [key, reader] : readers) {
            reader->open();
        }
        // ReSharper disable once CppUseElementsView
        for (const auto& [key, writer] : writers) {
            writer->open();
        }
    }

    bool StreamManager::hasDevice(const Mode mode, const Device device) const {
        if (mode == Capture) {
            return readers.contains(device);
        }
        return false;
    }

    void StreamManager::onFrame(const std::function<void(int64_t, Mode, Device, const bytes::binary&, wrtc::FrameData)>& callback) {
        frameCallback = callback;
    }

    bool StreamManager::updateMute(const bool isMuted) {
        std::lock_guard lock(mutex);
        bool changed = false;
        for (const auto& [key, track] : tracks) {
            if (key.first != Capture) {
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
            if (mode == Capture) {
                if (streamType == Audio) {
                    streams[id] = std::make_unique<AudioStreamer>();
                } else {
                    streams[id] = std::make_unique<VideoStreamer>();
                }
            } else {
                if (streamType == Audio) {
                    streams[id] = std::make_unique<AudioReceiver>();
                } else {
                    // TODO: Implement video receiver
                }
                dynamic_cast<BaseReceiver*>(streams[id].get())->open();
            }
        }

        if (desc) {
            auto sink = dynamic_cast<SinkType*>(streams[id].get());
            if (sink && sink->setConfig(desc) || !readers.contains(device) || !writers.contains(device)) {
                if (mode == Capture) {
                    const bool isShared = desc.value().mediaSource == DescriptionType::MediaSource::Device || desc.value().mediaSource == DescriptionType::MediaSource::Desktop;
                    readers[device] = MediaSourceFactory::fromInput(desc.value(), streams[id].get());
                    readers[device]->onData([this, id, streamType, isShared](const bytes::unique_binary& data, wrtc::FrameData frameData) {
                        if (streams.contains(id)) {
                            if (const auto stream = dynamic_cast<BaseStreamer*>(streams[id].get())) {
                                frameData.absoluteCaptureTimestampMs = rtc::TimeMillis();
                                if (streamType == Video && isShared) {
                                    (void) frameCallback(
                                        0,
                                        id.first,
                                        id.second,
                                        {data.get(), data.get() + streams[id]->frameSize()},
                                        frameData
                                    );
                                }
                                stream->sendData(data.get(), frameData);
                            }
                        }
                    });
                    readers[device]->onEof([this, device] {
                        workerThread->PostTask([this, device] {
                            (void) onEOF(getStreamType(device), device);
                            std::lock_guard lock(mutex);
                            readers.erase(device);
                        });
                    });
                    if (initialized) {
                        readers[device]->open();
                    }
                } else {
                    if (streamType == Audio) {
                        writers[device] = MediaSourceFactory::fromAudioOutput(desc.value(), streams[id].get());
                        dynamic_cast<AudioReceiver*>(streams[id].get())->onFrames([this, device](const std::map<uint32_t, bytes::unique_binary>& frames) {
                            if (writers.contains(device)) {
                                if (const auto audioWriter = dynamic_cast<AudioWriter*>(writers[device].get())) {
                                    audioWriter->sendFrames(frames);
                                }
                            }
                        });
                    } else {
                        // TODO: Implement video writer
                        throw InvalidParams("Video input is not yet supported");
                    }
                    writers[device]->onEof([this, device] {
                        workerThread->PostTask([this, device] {
                            std::lock_guard lock(mutex);
                            writers.erase(device);
                        });
                    });
                    if (initialized) {
                        writers[device]->open();
                    }
                }
            }
        } else if (mode == Capture) {
            readers.erase(device);
        } else {
            writers.erase(device);
        }
    }
} // ntgcalls