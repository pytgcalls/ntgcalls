//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <string>
#include <wrtc/wrtc.hpp>

#include "stream.hpp"
#include "models/auth_params.hpp"
#include "models/media_description.hpp"
#include "models/call_payload.hpp"
#include "models/rtc_server.hpp"
#include "utils/signaling_connection.hpp"


#define CHECK_CONNECTION_AND_THROW_ERROR() \
if (connection || g_a_or_b) { \
throw ConnectionError("Connection already made");\
}

namespace ntgcalls {
    using nlohmann::json;

    class Client {
        std::shared_ptr<wrtc::PeerConnection> connection;
        wrtc::SSRC audioSource = 0;
        std::vector<wrtc::SSRC> sourceGroups = {};
        std::shared_ptr<Stream> stream;
        bool connected = false;
        wrtc::synchronized_callback<void> onCloseConnection;
        wrtc::synchronized_callback<bytes::binary> onEmitData;
        std::shared_ptr<SignalingConnection> signaling;

        // P2P
        bytes::binary randomPower, prime, g_a_or_b, g_a_hash;

        CallPayload init(const std::vector<RTCServer>& servers = {});

        void processSignalingData(const bytes::binary& buffer) const;
    public:
        enum class Type {
            Group = 1 << 0,
            Outgoing = 1 << 1,
            Incoming = 1 << 2,
            P2P = Outgoing | Incoming,
            Unknown = 1 << 3
        };

        Client();

        ~Client();

        bytes::binary init(int32_t g, const bytes::binary& p, const bytes::binary& r, const bytes::binary& g_a_hash);

        AuthParams confirmConnection(const bytes::binary& p, const bytes::binary& g_a_or_b, const int64_t& fingerprint, const std::vector<RTCServer>& servers);

        std::string init(const MediaDescription& config);

        void connect(const std::string& jsonData);

        void changeStream(const MediaDescription& config) const;

        [[nodiscard]] bool pause() const;

        [[nodiscard]] bool resume() const;

        [[nodiscard]] bool mute() const;

        [[nodiscard]] bool unmute() const;

        void stop() const;

        [[nodiscard]] uint64_t time() const;

        [[nodiscard]] MediaState getState() const;

        [[nodiscard]] Stream::Status status() const;

        void onUpgrade(const std::function<void(MediaState)>& callback) const;

        void onStreamEnd(const std::function<void(Stream::Type)>& callback) const;

        void onDisconnect(const std::function<void()>& callback);

        void onSignalingData(const std::function<void(bytes::binary)>& callback);

        void sendSignalingData(const bytes::binary& buffer) const;

        Type callType() const;
    };
}
