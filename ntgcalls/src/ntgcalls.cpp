//
// Created by Laky64 on 22/08/2023.
//

#include <ntgcalls/ntgcalls.hpp>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/devices/media_device.hpp>
#include <ntgcalls/instances/group_call.hpp>
#include <ntgcalls/instances/p2p_call.hpp>
#include <ntgcalls/models/dh_config.hpp>
#include <ntgcalls/utils/g_lib_loop_manager.hpp>
#include <wrtc/video_factory/video_factory_config.hpp>

namespace ntgcalls {
    NTgCalls::NTgCalls() {
        updateThread = rtc::Thread::Create();
        updateThread->Start();
        hardwareInfo = std::make_unique<HardwareInfo>();
        INIT_ASYNC
        LogSink::GetOrCreate();
    }

    NTgCalls::~NTgCalls() {
#ifdef PYTHON_ENABLED
        py::gil_scoped_release release;
#endif
        std::unique_lock lock(mutex);
        RTC_LOG(LS_VERBOSE) << "Destroying NTgCalls";
        connections = {};
        hardwareInfo = nullptr;
        lock.unlock();
        updateThread->Stop();
        updateThread = nullptr;
        DESTROY_ASYNC
        RTC_LOG(LS_VERBOSE) << "NTgCalls destroyed";
        LogSink::UnRef();
    }

    void NTgCalls::setupListeners(const int64_t chatId) {
        connections[chatId]->onStreamEnd([this, chatId](const StreamManager::Type &type, const StreamManager::Device &device) {
            WORKER("onStreamEnd", updateThread, this, chatId, type, device)
            THREAD_SAFE
            (void) onEof(chatId, type, device);
            END_THREAD_SAFE
            END_WORKER
        });
        if (connections[chatId]->type() & CallInterface::Type::Group) {
            SafeCall<GroupCall>(connections[chatId].get())->onUpgrade([this, chatId](const MediaState &state) {
                WORKER("onUpgrade", updateThread, this, chatId, state)
                THREAD_SAFE
                (void) mediaStateCallback(chatId, state);
                END_THREAD_SAFE
                END_WORKER
            });
        }
        connections[chatId]->onConnectionChange([this, chatId](const CallNetworkState &state) {
            WORKER("onConnectionChange", updateThread, this, chatId, state)
            THREAD_SAFE
            if (state.kind == CallNetworkState::Kind::Normal) {
                switch (state.connectionState) {
                    case CallNetworkState::ConnectionState::Closed:
                    case CallNetworkState::ConnectionState::Failed:
                    case CallNetworkState::ConnectionState::Timeout:
                        remove(chatId);
                        break;
                    default:
                        break;
                }
            }
            (void) connectionChangeCallback(chatId, state);
            END_THREAD_SAFE
            END_WORKER
        });
        connections[chatId]->onFrames([this, chatId] (const StreamManager::Mode mode, const StreamManager::Device device, const std::vector<wrtc::Frame>& frames) {
            THREAD_SAFE
            (void) framesCallback(chatId, mode, device, frames);
            END_THREAD_SAFE
        });
        connections[chatId]->onRemoteSourceChange([this, chatId](const RemoteSource &state) {
            WORKER("onRemoteSourceChange", updateThread, this, chatId, state)
            THREAD_SAFE
            (void) remoteSourceCallback(chatId, state);
            END_THREAD_SAFE
            END_WORKER
        });
        if (connections[chatId]->type() & CallInterface::Type::P2P) {
            SafeCall<P2PCall>(connections[chatId].get())->onSignalingData([this, chatId](const bytes::binary& data) {
                WORKER("onSignalingData", updateThread, this, chatId, data)
                THREAD_SAFE
                (void) emitCallback(chatId, CAST_BYTES(data));
                END_THREAD_SAFE
                END_WORKER
            });
        }
    }

    ASYNC_RETURN(void) NTgCalls::createP2PCall(const int64_t userId, const MediaDescription& media) {
        SMART_ASYNC(this, media, userId)
        CHECK_AND_THROW_IF_EXISTS(userId)
        connections[userId] = std::make_shared<P2PCall>(updateThread.get());
        setupListeners(userId);
        SafeCall<P2PCall>(connections[userId].get())->init(media);
        END_ASYNC
    }

    ASYNC_RETURN(bytes::vector) NTgCalls::initExchange(const int64_t userId, const DhConfig& dhConfig, const std::optional<BYTES(bytes::vector)> &g_a_hash) {
        SMART_ASYNC(this, userId, dhConfig, g_a_hash = CPP_BYTES(g_a_hash, bytes::vector))
        const auto result = SafeCall<P2PCall>(safeConnection(userId))->initExchange(dhConfig, g_a_hash);
        THREAD_SAFE
        return CAST_BYTES(result);
        END_THREAD_SAFE
        END_ASYNC
    }

    ASYNC_RETURN(AuthParams) NTgCalls::exchangeKeys(const int64_t userId, const BYTES(bytes::vector) &g_a_or_b, const int64_t fingerprint) {
        SMART_ASYNC(this, userId, g_a_or_b = CPP_BYTES(g_a_or_b, bytes::vector), fingerprint)
        return SafeCall<P2PCall>(safeConnection(userId))->exchangeKeys(g_a_or_b, fingerprint);
        END_ASYNC
    }

    ASYNC_RETURN(void) NTgCalls::skipExchange(const int64_t userId, const BYTES(bytes::vector) &encryptionKey, const bool isOutgoing) {
        SMART_ASYNC(this, userId, encryptionKey = CPP_BYTES(encryptionKey, bytes::vector), isOutgoing)
        SafeCall<P2PCall>(safeConnection(userId))->skipExchange(encryptionKey, isOutgoing);
        END_ASYNC
    }

    ASYNC_RETURN(void) NTgCalls::connectP2P(const int64_t userId, const std::vector<RTCServer>& servers, const std::vector<std::string>& versions, const bool p2pAllowed) {
        SMART_ASYNC(this, userId, servers, versions, p2pAllowed)
        SafeCall<P2PCall>(safeConnection(userId))->connect(servers, versions, p2pAllowed);
        END_ASYNC
    }

    ASYNC_RETURN(std::string) NTgCalls::createCall(const int64_t chatId, const MediaDescription& media) {
        SMART_ASYNC(this, chatId, media)
        CHECK_AND_THROW_IF_EXISTS(chatId)
        connections[chatId] = std::make_shared<GroupCall>(updateThread.get());
        setupListeners(chatId);
        return SafeCall<GroupCall>(connections[chatId].get())->init(media);
        END_ASYNC
    }

    ASYNC_RETURN(std::string) NTgCalls::initPresentation(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        return SafeCall<GroupCall>(safeConnection(chatId))->initPresentation();
        END_ASYNC
    }

    ASYNC_RETURN(void) NTgCalls::connect(const int64_t chatId, const std::string& params, const bool isPresentation) {
        SMART_ASYNC(this, chatId, params, isPresentation)
        SafeCall<GroupCall>(safeConnection(chatId))->connect(params, isPresentation);
        END_ASYNC
    }

    ASYNC_RETURN(uint32_t) NTgCalls::addIncomingVideo(const int64_t chatId, const std::string& endpoint, const std::vector<wrtc::SsrcGroup>& ssrcGroups) {
        SMART_ASYNC(this, chatId, endpoint, ssrcGroups)
        return SafeCall<GroupCall>(safeConnection(chatId))->addIncomingVideo(endpoint, ssrcGroups);
        END_ASYNC
    }

    ASYNC_RETURN(bool) NTgCalls::removeIncomingVideo(const int64_t chatId, const std::string& endpoint) {
        SMART_ASYNC(this, chatId, endpoint)
        return SafeCall<GroupCall>(safeConnection(chatId))->removeIncomingVideo(endpoint);
        END_ASYNC
    }

    ASYNC_RETURN(void) NTgCalls::setStreamSources(const int64_t chatId, const StreamManager::Mode mode, const MediaDescription& media) {
        SMART_ASYNC(this, chatId, mode, media)
        safeConnection(chatId)->setStreamSources(mode, media);
        END_ASYNC
    }

    ASYNC_RETURN(bool) NTgCalls::pause(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        return safeConnection(chatId)->pause();
        END_ASYNC
    }

    ASYNC_RETURN(bool) NTgCalls::resume(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        return safeConnection(chatId)->resume();
        END_ASYNC
    }

    ASYNC_RETURN(bool) NTgCalls::mute(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        return safeConnection(chatId)->mute();
        END_ASYNC
    }

    ASYNC_RETURN(bool) NTgCalls::unmute(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        return safeConnection(chatId)->unmute();
        END_ASYNC
    }

    ASYNC_RETURN(void) NTgCalls::stop(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        remove(chatId);
        END_ASYNC
    }

    ASYNC_RETURN(void) NTgCalls::stopPresentation(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        SafeCall<GroupCall>(safeConnection(chatId))->stopPresentation(true);
        END_ASYNC
    }

    void NTgCalls::onStreamEnd(const std::function<void(int64_t, StreamManager::Type, StreamManager::Device)>& callback) {
        std::lock_guard lock(mutex);
        onEof = callback;
    }

    void NTgCalls::onUpgrade(const std::function<void(int64_t, MediaState)>& callback) {
        std::lock_guard lock(mutex);
        mediaStateCallback = callback;
    }

    void NTgCalls::onConnectionChange(const std::function<void(int64_t, CallNetworkState)>& callback) {
       std::lock_guard lock(mutex);
       connectionChangeCallback = callback;
    }

    void NTgCalls::onFrames(const std::function<void(int64_t, StreamManager::Mode, StreamManager::Device, const std::vector<wrtc::Frame>&)>& callback) {
        std::lock_guard lock(mutex);
        framesCallback = callback;
    }

    void NTgCalls::onSignalingData(const std::function<void(int64_t, const BYTES(bytes::binary)&)>& callback) {
        std::lock_guard lock(mutex);
        emitCallback = callback;
    }

    void NTgCalls::onRemoteSourceChange(const std::function<void(int64_t, RemoteSource)>& callback) {
        std::lock_guard lock(mutex);
        remoteSourceCallback = callback;
    }

    ASYNC_RETURN(void) NTgCalls::sendSignalingData(const int64_t chatId, const BYTES(bytes::binary) &msgKey) {
        SMART_ASYNC(this, chatId, msgKey = CPP_BYTES(msgKey, bytes::binary))
        SafeCall<P2PCall>(safeConnection(chatId))->sendSignalingData(msgKey);
        END_ASYNC
    }

    ASYNC_RETURN(void) NTgCalls::sendExternalFrame(const int64_t chatId, const StreamManager::Device device, const BYTES(bytes::binary) &data, const wrtc::FrameData frameData) {
        SMART_ASYNC(this, chatId, device, data = CPP_BYTES(data, bytes::binary), frameData)
        safeConnection(chatId)->sendExternalFrame(device, data, frameData);
        END_ASYNC
    }

    ASYNC_RETURN(uint64_t) NTgCalls::time(const int64_t chatId, const StreamManager::Mode mode) {
        SMART_ASYNC(this, chatId, mode)
        return safeConnection(chatId)->time(mode);
        END_ASYNC
    }

    ASYNC_RETURN(MediaState) NTgCalls::getState(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        return safeConnection(chatId)->getState();
        END_ASYNC
    }

    ASYNC_RETURN(double) NTgCalls::cpuUsage() {
        SMART_ASYNC(this)
        return hardwareInfo->getCpuUsage();
        END_ASYNC
    }

    ASYNC_RETURN(std::map<int64_t, StreamManager::MediaStatus>) NTgCalls::calls() {
        SMART_ASYNC(this)
        std::map<int64_t, StreamManager::MediaStatus> statusList;
        for (const auto& [fst, snd] : connections) {
            statusList.emplace(fst, StreamManager::MediaStatus{
                snd->status(StreamManager::Mode::Playback),
                snd->status(StreamManager::Mode::Capture)
            });
        }
        return statusList;
        END_ASYNC
    }

    void NTgCalls::remove(const int64_t chatId) {
        RTC_LOG(LS_INFO) << "Removing call " << chatId << ", Acquiring lock";
        std::lock_guard lock(mutex);
        RTC_LOG(LS_INFO) << "Lock acquired, removing call " << chatId;
        if (!exists(chatId)) {
            RTC_LOG(LS_ERROR) << "Call " << chatId << " not found";
            THROW_CONNECTION_NOT_FOUND(chatId)
        }
        connections.erase(connections.find(chatId));
        RTC_LOG(LS_INFO) << "Call " << chatId << " removed";
    }

    bool NTgCalls::exists(const int64_t chatId) const {
        return connections.contains(chatId);
    }

    CallInterface* NTgCalls::safeConnection(const int64_t chatId) {
        std::lock_guard lock(mutex);
        if (!exists(chatId)) {
            THROW_CONNECTION_NOT_FOUND(chatId)
        }
        return connections[chatId].get();
    }

    Protocol NTgCalls::getProtocol() {
        return {
            92,
            92,
            true,
            true,
            signaling::Signaling::SupportedVersions(),
        };
    }

#ifndef IS_ANDROID
    void NTgCalls::enableGlibLoop(const bool enable) {
        GLibLoopManager::EnableEventLoop(enable);
    }

    void NTgCalls::enableH264Encoder(const bool enable) {
        wrtc::VideoFactoryConfig::EnableH264Encoder(enable);
    }
#endif

    template<typename DestCallType, typename BaseCallType>
    DestCallType* NTgCalls::SafeCall(BaseCallType* call) {
        if (!call) {
            return nullptr;
        }
        if (auto* derivedCall = dynamic_cast<DestCallType*>(call)) {
            return derivedCall;
        }
        throw ConnectionError("Invalid call type");
    }

    std::string NTgCalls::ping() {
        return "pong";
    }

    MediaDevices NTgCalls::getMediaDevices() {
        auto devices = MediaDevice::GetAudioDevices();
        std::vector<DeviceInfo> microphones, speakers;
        for (const auto& device : devices) {
            if (json::parse(device.metadata)["is_microphone"]) {
                microphones.emplace_back(device.name, device.metadata);
            } else {
                speakers.emplace_back(device.name, device.metadata);
            }
        }
        return {
            microphones,
            speakers,
            MediaDevice::GetCameraDevices(),
            MediaDevice::GetScreenDevices()
        };
    }
} // ntgcalls