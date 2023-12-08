//
// Created by Laky64 on 22/08/2023.
//
#pragma once


#include <cstdint>
#include "client.hpp"
#include "models/media_description.hpp"

namespace ntgcalls {

    class NTgCalls {
        std::map<int64_t, std::shared_ptr<Client>> connections;
        wrtc::synchronized_callback<int64_t, Stream::Type> onEof;
        wrtc::synchronized_callback<int64_t, MediaState> onChangeStatus;

        bool exists(int64_t chatId) const;

        std::shared_ptr<Client> safeConnection(int64_t chatId);

    public:
        ~NTgCalls();

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

        static std::string ping();

        void onUpgrade(const std::function<void(int64_t, MediaState)>& callback);

        void onStreamEnd(const std::function<void(int64_t, Stream::Type)>& callback);

        std::map<int64_t, Stream::Status> calls() const;
    };

} // ntgcalls

