//
// Created by Laky64 on 22/08/2023.
//
#pragma once


#include <cstdint>

#include <ntgcalls/instances/call_interface.hpp>
// ReSharper disable once CppUnusedIncludeDirective
#include <ntgcalls/models/auth_params.hpp>
#include <ntgcalls/models/dh_config.hpp>
#include <ntgcalls/models/protocol.hpp>
#include <ntgcalls/models/rtc_server.hpp>
#include <ntgcalls/utils/binding_utils.hpp>
#include <ntgcalls/utils/hardware_info.hpp>
#include <ntgcalls/utils/log_sink_impl.hpp>
#include <ntgcalls/devices/media_devices.hpp>
#include <ntgcalls/models/remote_source_state.hpp>
#include <wrtc/models/media_content.hpp>

#define CHECK_AND_THROW_IF_EXISTS(chatId) \
if (exists(chatId)) { \
throw ConnectionError("Connection cannot be initialized more than once."); \
}

#define THROW_CONNECTION_NOT_FOUND(chatId) \
throw ConnectionNotFound("Connection with chat id \"" + std::to_string(chatId) + "\" not found");

namespace ntgcalls {

    class NTgCalls {
        std::unordered_map<int64_t, std::shared_ptr<CallInterface>> connections;
        wrtc::synchronized_callback<int64_t, StreamManager::Type, StreamManager::Device> onEof;
        wrtc::synchronized_callback<int64_t, MediaState> mediaStateCallback;
        wrtc::synchronized_callback<int64_t, CallNetworkState> connectionChangeCallback;
        wrtc::synchronized_callback<int64_t, BYTES(bytes::binary)> emitCallback;
        wrtc::synchronized_callback<int64_t, RemoteSource> remoteSourceCallback;
        wrtc::synchronized_callback<int64_t, StreamManager::Mode, StreamManager::Device, std::vector<wrtc::Frame>> framesCallback;
        std::unique_ptr<rtc::Thread> updateThread;
        std::unique_ptr<HardwareInfo> hardwareInfo;
        std::mutex mutex;
        ASYNC_ARGS

        bool exists(int64_t chatId) const;

        CallInterface* safeConnection(int64_t chatId);

        void setupListeners(int64_t chatId);

        template<typename DestCallType, typename BaseCallType>
        static DestCallType* SafeCall(BaseCallType* call);

        void remove(int64_t chatId);

    public:
        explicit NTgCalls();

        ~NTgCalls();

        ASYNC_RETURN(void) createP2PCall(int64_t userId, const MediaDescription& media);

        ASYNC_RETURN(bytes::vector) initExchange(int64_t userId, const DhConfig& dhConfig, const std::optional<BYTES(bytes::vector)> &g_a_hash);

        ASYNC_RETURN(AuthParams) exchangeKeys(int64_t userId, const BYTES(bytes::vector) &g_a_or_b, int64_t fingerprint);

        ASYNC_RETURN(void) skipExchange(int64_t userId, const BYTES(bytes::vector) &encryptionKey, bool isOutgoing);

        ASYNC_RETURN(void) connectP2P(int64_t userId, const std::vector<RTCServer>& servers, const std::vector<std::string>& versions, bool p2pAllowed);

        ASYNC_RETURN(std::string) createCall(int64_t chatId, const MediaDescription& media);

        ASYNC_RETURN(std::string) initPresentation(int64_t chatId);

        ASYNC_RETURN(void) connect(int64_t chatId, const std::string& params, bool isPresentation);

        ASYNC_RETURN(uint32_t) addIncomingVideo(int64_t chatId, const std::string& endpoint, const std::vector<wrtc::SsrcGroup>& ssrcGroups);

        ASYNC_RETURN(bool) removeIncomingVideo(int64_t chatId, const std::string& endpoint);

        ASYNC_RETURN(void) setStreamSources(int64_t chatId, StreamManager::Mode mode, const MediaDescription& media);

        ASYNC_RETURN(bool) pause(int64_t chatId);

        ASYNC_RETURN(bool) resume(int64_t chatId);

        ASYNC_RETURN(bool) mute(int64_t chatId);

        ASYNC_RETURN(bool) unmute(int64_t chatId);

        ASYNC_RETURN(void) stop(int64_t chatId);

        ASYNC_RETURN(void) stopPresentation(int64_t chatId);

        ASYNC_RETURN(uint64_t) time(int64_t chatId, StreamManager::Mode mode);

        ASYNC_RETURN(MediaState) getState(int64_t chatId);

        ASYNC_RETURN(double) cpuUsage();

        static std::string ping();

        static MediaDevices getMediaDevices();

        static Protocol getProtocol();

#ifndef IS_ANDROID
        static void enableGlibLoop(bool enable);

        static void enableH264Encoder(bool enable);
#endif

        void onUpgrade(const std::function<void(int64_t, MediaState)>& callback);

        void onStreamEnd(const std::function<void(int64_t, StreamManager::Type, StreamManager::Device)>& callback);

        void onConnectionChange(const std::function<void(int64_t, CallNetworkState)>& callback);

        void onFrames(const std::function<void(int64_t, StreamManager::Mode, StreamManager::Device, const std::vector<wrtc::Frame>&)>& callback);

        void onSignalingData(const std::function<void(int64_t, const BYTES(bytes::binary)&)>& callback);

        void onRemoteSourceChange(const std::function<void(int64_t, RemoteSource)>& callback);

        ASYNC_RETURN(void) sendSignalingData(int64_t chatId, const BYTES(bytes::binary) &msgKey);

        ASYNC_RETURN(void) sendExternalFrame(int64_t chatId, StreamManager::Device device, const BYTES(bytes::binary) &data, wrtc::FrameData frameData);

        ASYNC_RETURN(std::map<int64_t, StreamManager::MediaStatus>) calls();
    };

} // ntgcalls
