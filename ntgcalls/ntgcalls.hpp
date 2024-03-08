//
// Created by Laky64 on 22/08/2023.
//
#pragma once


#include <cstdint>
#include "client.hpp"
#include "models/media_description.hpp"
#include "utils/hardware_info.hpp"

namespace ntgcalls {

#define CHECK_AND_THROW_IF_EXISTS(userId) \
if (exists(userId)) { \
throw ConnectionError("Connection cannot be initialized more than once."); \
}

    class NTgCalls {
        std::unordered_map<int64_t, std::shared_ptr<Client>> connections;
        wrtc::synchronized_callback<int64_t, Stream::Type> onEof;
        wrtc::synchronized_callback<int64_t, MediaState> onChangeStatus;
        wrtc::synchronized_callback<int64_t> onCloseConnection;
        std::shared_ptr<DispatchQueue> updateQueue;
        std::shared_ptr<HardwareInfo> hardwareInfo;
        std::mutex mutex;

        bool exists(int64_t chatId) const;

        std::shared_ptr<Client> safeConnection(int64_t chatId);

        void setupListeners(int64_t chatId);

    public:
        NTgCalls();

        ~NTgCalls();

        bytes::binary createP2PCall(int64_t userId, int32_t g, const bytes::binary& p, const bytes::binary& r, const bytes::binary& g_a_hash);

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

        std::map<int64_t, Stream::Status> calls();
    };

} // ntgcalls
