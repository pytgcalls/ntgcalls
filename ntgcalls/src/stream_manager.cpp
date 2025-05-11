//
// Created by Laky64 on 28/09/24.
//

#include <ranges>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/stream_manager.hpp>
#include <ntgcalls/io/threaded_reader.hpp>
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

    void StreamManager::close() {
        std::lock_guard lock(mutex);
        syncReaders.clear();
        syncCV.notify_all();
        onEOF = nullptr;
        framesCallback = nullptr;
        onChangeStatus = nullptr;
        for (const auto& reader : readers | std::views::values) {
            reader->onData(nullptr);
            reader->onEof(nullptr);
        }
        readers.clear();
        writers.clear();
        for (const auto& stream : streams | std::views::values) {
            if (const auto audioReceiver = dynamic_cast<AudioReceiver*>(stream.get())) {
                audioReceiver->onFrames(nullptr);
            } else if (const auto videoReceiver = dynamic_cast<VideoReceiver*>(stream.get())) {
                videoReceiver->onFrame(nullptr);
            }
        }
        streams.clear();
        tracks.clear();
        workerThread = nullptr;
    }

    void StreamManager::enableVideoSimulcast(const bool enable) {
       videoSimulcast = enable;
    }

    void StreamManager::setStreamSources(const Mode mode, const MediaDescription& desc) {
        RTC_LOG(LS_VERBOSE) << "Setting Configuration, Acquiring lock";
        std::lock_guard lock(mutex);
        RTC_LOG(LS_VERBOSE) << "Setting Configuration, Lock acquired";

        const bool wasIdling = isPaused();

        setConfig<AudioSink, AudioDescription>(mode, Microphone, desc.microphone);
        setConfig<AudioSink, AudioDescription>(mode, Speaker, desc.speaker);

        const bool wasCamera = hasDeviceInternal(mode, Camera);
        const bool wasScreen = hasDeviceInternal(mode, Screen);

        if (!videoSimulcast && desc.camera && desc.screen && mode == Capture) {
            throw InvalidParams("Cannot mix camera and screen sources");
        }

        setConfig<VideoSink, VideoDescription>(mode, Camera, desc.camera);
        setConfig<VideoSink, VideoDescription>(mode, Screen, desc.screen);

        if (mode == Capture && (wasCamera != hasDeviceInternal(mode, Camera) || wasScreen != hasDeviceInternal(mode, Screen) || wasIdling) && initialized) {
            checkUpgrade();
        }
    }

    void StreamManager::optimizeSources(wrtc::NetworkInterface* pc) {
        pc->enableAudioIncoming(writers.contains(Microphone) || externalWriters.contains(Microphone));
        pc->enableVideoIncoming(writers.contains(Camera) || externalWriters.contains(Camera), false);
        pc->enableVideoIncoming(writers.contains(Screen) || externalWriters.contains(Screen), true);
        initialized = pc->getConnectionMode() != wrtc::ConnectionMode::None;
    }

    MediaState StreamManager::getState() {
        std::lock_guard lock(mutex);
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
            !hasDeviceInternal(Capture, Camera),
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
        std::lock_guard lock(mutex);
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
        std::lock_guard lock(mutex);
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
        for (const auto& reader : readers | std::views::values) {
            reader->open();
        }
        for (const auto& writer : writers | std::views::values) {
            writer->open();
        }
    }

    bool StreamManager::hasDevice(const Mode mode, const Device device) {
        std::lock_guard lock(mutex);
        return hasDeviceInternal(mode, device);
    }

    bool StreamManager::hasReaders() {
        std::lock_guard lock(mutex);
        return !readers.empty();
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
            stream->sendData(uniqueData.get(), data.size(), frameData);
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
        const auto now = std::chrono::steady_clock::now();
        for (const auto& reader : readers | std::views::values) {
            if (reader->set_enabled(!isPaused)) {
                res = true;
            }
            if (const auto threadedReader = dynamic_cast<wrtc::SyncHelper*>(reader.get())) {
                threadedReader->synchronizeTime(now);
            }
        }
        if (res) {
            checkUpgrade();
        }
        return res;
    }

    bool StreamManager::isPaused() {
        auto res = false;
        for (const auto& reader : readers | std::views::values) {
            if (!reader->is_enabled()) {
                res = true;
            }
        }
        return res;
    }

    bool StreamManager::hasDeviceInternal(const Mode mode, const Device device) const {
        if (mode == Capture) {
            return readers.contains(device) || externalReaders.contains(device);
        }
        return writers.contains(device) || externalWriters.contains(device);
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
        std::weak_ptr weak(shared_from_this());
        workerThread->PostTask([weak] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
           (void) strong->onChangeStatus(strong->getState());
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
                    streams[id] = std::make_unique<VideoReceiver>();
                }
                dynamic_cast<BaseReceiver*>(streams[id].get())->open();
            }
        }

        if (desc) {
            auto sink = dynamic_cast<SinkType*>(streams[id].get());
            if (sink && sink->setConfig(desc) || !readers.contains(device) || !writers.contains(device) || !externalWriters.contains(device) && desc.value().mediaSource == DescriptionType::MediaSource::External) {
                if (mode == Capture) {
                    const bool isShared = desc.value().mediaSource == DescriptionType::MediaSource::Device;
                    if (readers.contains(device)) {
                        cancelSyncReaders.insert(device);
                        syncCV.notify_all();
                        readers[device]->onData(nullptr);
                        readers[device]->onEof(nullptr);
                    }
                    readers.erase(device);
                    if (cancelSyncReaders.contains(device)) {
                        cancelSyncReaders.erase(device);
                    }
                    if (desc.value().mediaSource == DescriptionType::MediaSource::External) {
                        externalReaders.insert(device);
                        syncReaders.insert(device);
                        return;
                    }
                    readers[device] = MediaSourceFactory::fromInput(desc.value(), streams[id].get());
                    syncReaders.insert(device);
                    std::weak_ptr weak(shared_from_this());
                    readers[device]->onData([weak, id, streamType, isShared](const bytes::unique_binary& data, wrtc::FrameData frameData) {
                        const auto strong = weak.lock();
                        if (!strong) {
                            return;
                        }
                        if (strong->syncReaders.contains(id.second)) {
                            std::unique_lock lock(strong->syncMutex);
                            strong->syncReaders.erase(id.second);
                            strong->syncCV.notify_all();
                            strong->syncCV.wait(lock, [strong, id] {
                                return strong->syncReaders.empty() || strong->cancelSyncReaders.contains(id.second);
                            });
                            if (strong->cancelSyncReaders.contains(id.second)) {
                                strong->cancelSyncReaders.erase(id.second);
                                return;
                            }
                            if (const auto threadedReader = dynamic_cast<wrtc::SyncHelper*>(strong->readers[id.second].get())) {
                                threadedReader->synchronizeTime();
                            }
                        }
                        if (strong->streams.contains(id)) {
                            const auto frameSize = strong->streams[id]->frameSize();
                            if (const auto stream = dynamic_cast<BaseStreamer*>(strong->streams[id].get())) {
                                frameData.absoluteCaptureTimestampMs = rtc::TimeMillis();
                                if (streamType == Video && isShared) {
                                    (void) strong->framesCallback(
                                        id.first,
                                        id.second,
                                        {
                                            {
                                                0,
                                                {data.get(), data.get() + frameSize},
                                                frameData
                                            }
                                        }
                                    );
                                }
                                stream->sendData(data.get(), frameSize, frameData);
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
                            std::lock_guard lock(strongThread->mutex);
                            if (strongThread->syncReaders.contains(device)) {
                                strongThread->syncReaders.erase(device);
                                strongThread->cancelSyncReaders.insert(device);
                                strongThread->syncCV.notify_all();
                            }
                            if (strongThread->readers.contains(device)) {
                                strongThread->readers[device]->onData(nullptr);
                                strongThread->readers[device]->onEof(nullptr);
                                strongThread->readers.erase(device);
                            }
                            if (strongThread->cancelSyncReaders.contains(device)) {
                                strongThread->cancelSyncReaders.erase(device);
                            }
                            (void) strongThread->onEOF(getStreamType(device), device);
                        });
                    });
                    if (initialized) {
                        readers[device]->open();
                        RTC_LOG(LS_VERBOSE) << "Reader opened";
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
            if (syncReaders.contains(device)) {
                syncReaders.erase(device);
                syncCV.notify_all();
            }
            if (readers.contains(device)) {
                readers[device]->onData(nullptr);
                readers[device]->onEof(nullptr);
            }
            readers.erase(device);
            externalReaders.erase(device);
        } else {
            writers.erase(device);
            externalWriters.erase(device);
        }
    }
} // ntgcalls