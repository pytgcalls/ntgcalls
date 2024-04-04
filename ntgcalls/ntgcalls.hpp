//
// Created by Laky64 on 22/08/2023.
//
#pragma once


#include <cstdint>

#include "instances/call_interface.hpp"
// ReSharper disable once CppUnusedIncludeDirective
#include "models/auth_params.hpp"
#include "models/protocol.hpp"
#include "models/rtc_server.hpp"
#include "utils/binding_utils.hpp"
#include "utils/hardware_info.hpp"
#include "utils/log_sink_impl.hpp"

#define CHECK_AND_THROW_IF_EXISTS(chatId) \
if (exists(chatId)) { \
throw ConnectionError("Connection cannot be initialized more than once."); \
}

#define THROW_CONNECTION_NOT_FOUND(chatId) \
throw ConnectionNotFound("Connection with chat id \"" + std::to_string(chatId) + "\" not found");

namespace ntgcalls {

    class NTgCalls {
        std::unordered_map<int64_t, std::shared_ptr<CallInterface>> connections;
        wrtc::synchronized_callback<int64_t, Stream::Type> onEof;
        wrtc::synchronized_callback<int64_t, MediaState> onChangeStatus;
        wrtc::synchronized_callback<int64_t> onCloseConnection;
        wrtc::synchronized_callback<int64_t, BYTES(bytes::binary)> onEmitData;
        std::unique_ptr<rtc::Thread> workerThread;
        std::unique_ptr<rtc::Thread> updateThread;
        std::unique_ptr<rtc::Thread> networkThread;
        std::unique_ptr<HardwareInfo> hardwareInfo;
        std::mutex mutex;
        ASYNC_ARGS

        bool exists(int64_t chatId) const;

        std::shared_ptr<CallInterface> safeConnection(int64_t chatId);

        void setupListeners(int64_t chatId);

        template<typename DestCallType, typename BaseCallType>
        static DestCallType* SafeCall(const std::shared_ptr<BaseCallType>& call);

        void remove(int64_t chatId);

    public:
        explicit NTgCalls();

        ~NTgCalls();

        ASYNC_RETURN(bytes::vector) createP2PCall(int64_t userId, const int32_t &g, const BYTES(bytes::vector) &p, const BYTES(bytes::vector) &r, const std::optional<BYTES(bytes::vector)> &g_a_hash, const MediaDescription& media);

        ASYNC_RETURN(AuthParams) exchangeKeys(int64_t userId, const BYTES(bytes::vector) &g_a_or_b, int64_t fingerprint);

        ASYNC_RETURN(void) connectP2P(int64_t userId, const std::vector<RTCServer>& servers, const std::vector<std::string>& versions, bool p2pAllowed);

        ASYNC_RETURN(std::string) createCall(int64_t chatId, const MediaDescription& media);

        ASYNC_RETURN(void) connect(int64_t chatId, const std::string& params);

        ASYNC_RETURN(void) changeStream(int64_t chatId, const MediaDescription& media);

        ASYNC_RETURN(bool) pause(int64_t chatId);

        ASYNC_RETURN(bool) resume(int64_t chatId);

        ASYNC_RETURN(bool) mute(int64_t chatId);

        ASYNC_RETURN(bool) unmute(int64_t chatId);

        ASYNC_RETURN(void) stop(int64_t chatId);

        ASYNC_RETURN(uint64_t) time(int64_t chatId);

        ASYNC_RETURN(MediaState) getState(int64_t chatId);

        ASYNC_RETURN(double) cpuUsage() const;

        static std::string ping();

        static Protocol getProtocol();

        void onUpgrade(const std::function<void(int64_t, MediaState)>& callback);

        void onStreamEnd(const std::function<void(int64_t, Stream::Type)>& callback);

        void onDisconnect(const std::function<void(int64_t)>& callback);

        void onSignalingData(const std::function<void(int64_t, const BYTES(bytes::binary)&)>& callback);

        ASYNC_RETURN(void) sendSignalingData(int64_t chatId, const BYTES(bytes::binary) &msgKey);

        ASYNC_RETURN(std::map<int64_t, Stream::Status>) calls();
    };

} // ntgcalls
