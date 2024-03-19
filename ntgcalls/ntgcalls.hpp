//
// Created by Laky64 on 22/08/2023.
//
#pragma once


#include <cstdint>
#include <future>

#include "instances/call_interface.hpp"
#include "models/auth_params.hpp"
#include "models/protocol.hpp"
#include "../wrtc/models/rtc_server.hpp"
#include "utils/binding_utils.hpp"
#include "utils/hardware_info.hpp"

#define CHECK_AND_THROW_IF_EXISTS(chatId) \
if (exists(chatId)) { \
throw ConnectionError("Connection cannot be initialized more than once."); \
}

namespace ntgcalls {

    class NTgCalls {
        std::unordered_map<int64_t, std::shared_ptr<CallInterface>> connections;
        wrtc::synchronized_callback<int64_t, Stream::Type> onEof;
        wrtc::synchronized_callback<int64_t, MediaState> onChangeStatus;
        wrtc::synchronized_callback<int64_t> onCloseConnection;
        wrtc::synchronized_callback<int64_t, bytes::binary> onEmitData;
        std::unique_ptr<DispatchQueue> updateQueue;
        std::unique_ptr<HardwareInfo> hardwareInfo;
        std::mutex mutex;

        bool exists(int64_t chatId) const;

        std::shared_ptr<CallInterface> safeConnection(int64_t chatId);

        void setupListeners(int64_t chatId);

        template<typename DestCallType, typename BaseCallType>
        static DestCallType* SafeCall(const std::shared_ptr<BaseCallType>& call);

    public:
        NTgCalls();

        ~NTgCalls();

        bytes::vector createP2PCall(int64_t userId, const int32_t &g, const bytes::vector &p, const bytes::vector &r, const std::optional<bytes::vector> &g_a_hash);

        AuthParams exchangeKeys(int64_t userId, const bytes::vector &p, const bytes::vector &g_a_or_b, int64_t fingerprint);

        void connectP2P(int64_t userId, const std::vector<wrtc::RTCServer>& servers, const std::vector<std::string>& versions);

        std::string createCall(int64_t chatId, const MediaDescription& media);

        void connect(int64_t chatId, const std::string& params);

        void changeStream(int64_t chatId, const MediaDescription& media);

        bool pause(int64_t chatId);

        bool resume(int64_t chatId);

        bool mute(int64_t chatId);

        bool unmute(int64_t chatId);

        void stop(int64_t chatId);

        uint64_t time(int64_t chatId);

        MediaState getState(int64_t chatId);

        double cpuUsage() const;

        static std::string ping();

        static Protocol getProtocol();

        void onUpgrade(const std::function<void(int64_t, MediaState)>& callback);

        void onStreamEnd(const std::function<void(int64_t, Stream::Type)>& callback);

        void onDisconnect(const std::function<void(int64_t)>& callback);

        void onSignalingData(const std::function<void(int64_t, const bytes::binary&)>& callback);

        void sendSignalingData(int64_t chatId, const bytes::binary &msgKey);

        std::map<int64_t, Stream::Status> calls();
    };

} // ntgcalls
