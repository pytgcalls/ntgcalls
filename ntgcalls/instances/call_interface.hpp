//
// Created by Laky64 on 15/03/2024.
//

#pragma once
#include <memory>

#include "ntgcalls/stream.hpp"

namespace ntgcalls {

    class CallInterface {
    protected:
        std::shared_ptr<wrtc::PeerConnection> connection;
        std::shared_ptr<Stream> stream;
        bool connected = false;
        wrtc::synchronized_callback<void> onCloseConnection;
    public:
        CallInterface();

        virtual ~CallInterface();

        enum class Type {
            Group = 1 << 0,
            Outgoing = 1 << 1,
            Incoming = 1 << 2,
            P2P = Outgoing | Incoming
        };

        bool pause() const;

        bool resume() const;

        bool mute() const;

        bool unmute() const;

        void stop() const;

        void changeStream(const MediaDescription& config) const;

        void onStreamEnd(const std::function<void(Stream::Type)> &callback) const;

        void onDisconnect(const std::function<void()> &callback);

        void onUpgrade(const std::function<void(MediaState)> &callback) const;

        uint64_t time() const;

        MediaState getState() const;

        Stream::Status status() const;

        virtual Type type() const = 0;
    };

    inline int operator&(const CallInterface::Type& lhs, const CallInterface::Type rhs){
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }
} // ntgcalls
