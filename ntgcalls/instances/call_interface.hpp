//
// Created by Laky64 on 15/03/2024.
//

#pragma once
#include <memory>

#include "ntgcalls/stream.hpp"

namespace ntgcalls {

    class CallInterface {
    protected:
        std::mutex mutex;
        std::unique_ptr<wrtc::PeerConnection> connection;
        std::unique_ptr<Stream> stream;
        bool connected = false;
        wrtc::synchronized_callback<void> onCloseConnection;
    public:
        explicit CallInterface(rtc::Thread* workerThread);

        virtual ~CallInterface();

        enum class Type {
            Group = 1 << 0,
            Outgoing = 1 << 1,
            Incoming = 1 << 2,
            P2P = Outgoing | Incoming
        };

        bool pause();

        bool resume();

        bool mute();

        bool unmute();

        void changeStream(const MediaDescription& config);

        void onStreamEnd(const std::function<void(Stream::Type)> &callback);

        void onDisconnect(const std::function<void()> &callback);

        uint64_t time();

        MediaState getState();

        Stream::Status status();

        virtual Type type() const = 0;
    };

    inline int operator&(const CallInterface::Type& lhs, const CallInterface::Type rhs){
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }
} // ntgcalls
