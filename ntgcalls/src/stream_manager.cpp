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
#include <ntgcalls/media/video_receiver.hpp>
#include <ntgcalls/media/video_sink.hpp>
#include <ntgcalls/media/video_streamer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls {

    StreamManager::StreamManager(rtc::Thread* workerThread): workerThread(workerThread) {}

    StreamManager::~StreamManager() {
        RTC_LOG(LS_VERBOSE) << "Destroying Stream";
        workerThread->BlockingCall([this] {
            RTC_LOG(LS_VERBOSE) << "Destroying Stream, Acquiring lock";
            std::lock_guard lock(mutex);
            RTC_LOG(LS_VERBOSE) << "Destroying Stream, Lock acquired";
            syncReaders.clear();
            RTC_LOG(LS_VERBOSE) << "Destroying Stream, Notifying all";
            syncCV.notify_all();
            RTC_LOG(LS_VERBOSE) << "End of Stream destruction";
            onEOF = nullptr;
            RTC_LOG(LS_VERBOSE) << "Removing I/O streams";
            readers.clear();
            writers.clear();
            RTC_LOG(LS_VERBOSE) << "Removing streams and tracks";
            streams.clear();
            tracks.clear();
        });
        RTC_LOG(LS_VERBOSE) << "Cleaning up";
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

        if (!videoSimulcast && desc.camera && desc.screen && mode == Capture) {
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
        RTC_LOG(LS_INFO) << "Configuration set";
    }

    void StreamManager::optimizeSources(wrtc::NetworkInterface* pc) const {
        pc->enableAudioIncoming(writers.contains(Microphone) || externalWriters.contains(Microphone));
        pc->enableVideoIncoming(writers.contains(Camera) || externalWriters.contains(Camera), false);
        pc->enableVideoIncoming(writers.contains(Screen) || externalWriters.contains(Screen), true);
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
        if (count == 0) {
            return 0;
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

    void StreamManager::addTrack(Mode mode, Device device, wrtc::NetworkInterface* pc) {
        const std::pair id(mode, device);
        if (mode == Capture) {
            tracks[id] = pc->addOutgoingTrack(dynamic_cast<BaseStreamer*>(streams[id].get())->createTrack());
        } else {
            if (id.second == Microphone || id.second == Speaker) {
                pc->addIncomingAudioTrack(dynamic_cast<AudioReceiver*>(streams[id].get())->remoteSink());
            } else {
                pc->addIncomingVideoTrack(dynamic_cast<VideoReceiver*>(streams[id].get())->remoteSink(), id.second == Screen);
            }
        }
    }

    void StreamManager::start() {
        std::lock_guard lock(mutex);
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

    void StreamManager::onFrames(const std::function<void(Mode, Device, const std::vector<wrtc::Frame>&)>& callback) {
        framesCallback = callback;
    }

    void StreamManager::sendExternalFrame(Device device, const bytes::binary& data, const wrtc::FrameData frameData) {
        const std::pair id(Capture, device);
        if (!externalReaders.contains(device) || !streams.contains(id)) {
            throw InvalidParams("External source not initialized");
        }
        if (const auto stream = dynamic_cast<BaseStreamer*>(streams[id].get())) {
            const auto uniqueData = bytes::make_unique_binary(data.size());
            memcpy(uniqueData.get(), data.data(), data.size());
            stream->sendData(uniqueData.get(), frameData);
        }
    }

    bool StreamManager::updateMute(const bool isMuted) {
        std::lock_guard lock(mutex);
        bool changed = false;
        for (const auto& [key, track] : tracks) {
            if (key.first == Playback || key.second == Camera || key.second == Screen) {
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
            RTC_LOG(LS_INFO) << "Ignoring screen source";
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
                    streams[id] = std::make_unique<VideoReceiver>();
                }
                dynamic_cast<BaseReceiver*>(streams[id].get())->open();
            }
            RTC_LOG(LS_INFO) << "Stream created";
        }

        if (desc) {
            auto sink = dynamic_cast<SinkType*>(streams[id].get());
            if (sink && sink->setConfig(desc) || !readers.contains(device) || !writers.contains(device) || !externalWriters.contains(device)) {
                if (mode == Capture) {
                    RTC_LOG(LS_INFO) << "Setting reader";
                    const bool isShared = desc.value().mediaSource == DescriptionType::MediaSource::Device;
                    readers.erase(device);
                    RTC_LOG(LS_INFO) << "Reader erased";
                    if (desc.value().mediaSource == DescriptionType::MediaSource::External) {
                        externalReaders.insert(device);
                        syncReaders.insert(device);
                        return;
                    }
                    readers[device] = MediaSourceFactory::fromInput(desc.value(), streams[id].get());
                    RTC_LOG(LS_INFO) << "Reader created from input";
                    syncReaders.insert(device);
                    std::weak_ptr weak(shared_from_this());
                    readers[device]->onData([weak, id, streamType, isShared](const bytes::unique_binary& data, wrtc::FrameData frameData) {
                        frameData.absoluteCaptureTimestampMs = rtc::TimeMillis();
                        const auto strong = weak.lock();
                        if (!strong) {
                            return;
                        }
                        if (strong->syncReaders.contains(id.second)) {
                            std::unique_lock lock(strong->syncMutex);
                            strong->syncReaders.erase(id.second);
                            strong->syncCV.notify_all();
                            strong->syncCV.wait(lock, [strong]{
                                return strong->syncReaders.empty();
                            });
                        }
                        if (strong->streams.contains(id)) {
                            if (const auto stream = dynamic_cast<BaseStreamer*>(strong->streams[id].get())) {
                                if (streamType == Video && isShared) {
                                    (void) strong->framesCallback(
                                        id.first,
                                        id.second,
                                        {
                                            {
                                                0,
                                                {data.get(), data.get() + strong->streams[id]->frameSize()},
                                                frameData
                                            }
                                        }
                                    );
                                }
                                stream->sendData(data.get(), frameData);
                            }
                        }
                    });
                    readers[device]->onEof([weak, device] {
                        const auto strong = weak.lock();
                        if (!strong) {
                            return;
                        }
                        strong->workerThread->PostTask([weak, device] {
                            const auto strongThread = weak.lock();
                            if (!strongThread) {
                                return;
                            }
                            if (strongThread->syncReaders.contains(device)) {
                                strongThread->syncReaders.erase(device);
                                strongThread->syncCV.notify_all();
                            }
                            (void) strongThread->onEOF(getStreamType(device), device);
                        });
                    });
                    if (initialized) {
                        readers[device]->open();
                        RTC_LOG(LS_INFO) << "Reader opened";
                    }
                } else {
                    const bool isExternal = desc.value().mediaSource == DescriptionType::MediaSource::External;
                    if (isExternal) {
                        externalWriters.insert(device);
                    }
                    if (streamType == Audio) {
                        if (!isExternal) {
                            writers.erase(device);
                            writers[device] = MediaSourceFactory::fromAudioOutput(desc.value(), streams[id].get());
                        }
                        std::weak_ptr weak(shared_from_this());
                        dynamic_cast<AudioReceiver*>(streams[id].get())->onFrames([weak, id, isExternal](const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) {
                            const auto strong = weak.lock();
                            if (!strong) {
                                return;
                            }
                            if (isExternal) {
                                std::vector<wrtc::Frame> externalFrames;
                                for (const auto& [ssrc, data] : frames) {
                                    if (strong->externalWriters.contains(id.second)) {
                                        externalFrames.push_back({
                                            ssrc,
                                            {data.first.get(), data.first.get() + data.second},
                                            {}
                                        });
                                    }
                                }
                                (void) strong->framesCallback(
                                    id.first,
                                    id.second,
                                    externalFrames
                                );
                            } else {
                                if (strong->writers.contains(id.second)) {
                                    if (const auto audioWriter = dynamic_cast<AudioWriter*>(strong->writers[id.second].get())) {
                                        audioWriter->sendFrames(frames);
                                    }
                                }
                            }
                        });
                    } else if (isExternal) {
                        std::weak_ptr weak(shared_from_this());
                        dynamic_cast<VideoReceiver*>(streams[id].get())->onFrame([weak, id](const uint32_t ssrc, const bytes::unique_binary& frame, const size_t size, const wrtc::FrameData frameData) {
                            const auto strong = weak.lock();
                            if (!strong) {
                                return;
                            }
                            if (strong->externalWriters.contains(id.second)) {
                                (void) strong->framesCallback(
                                    id.first,
                                    id.second,
                                    {
                                        {
                                            ssrc,
                                            {frame.get(), frame.get() + size},
                                            frameData
                                        }
                                    }
                                );
                            }
                        });
                    } else {
                        throw InvalidParams("Invalid input mode");
                    }
                    if (!isExternal) {
                        if (initialized) {
                            writers[device]->open();
                        }
                    }
                }
            }
        } else if (mode == Capture) {
            RTC_LOG(LS_INFO) << "Removing reader";
            if (syncReaders.contains(device)) {
                syncReaders.erase(device);
                syncCV.notify_all();
            }
            readers.erase(device);
            externalReaders.erase(device);
            RTC_LOG(LS_INFO) << "Reader removed";
        } else {
            writers.erase(device);
            externalWriters.erase(device);
        }
    }
} // ntgcalls