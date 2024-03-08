//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <string>
#include <wrtc/wrtc.hpp>

#include "stream.hpp"
#include "models/auth_params.hpp"
#include "models/media_description.hpp"
#include "models/group_call_payload.hpp"


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

        // P2P
        bytes::binary randomPower, prime, g_a_or_b, g_a_hash;

        GroupCallPayload init();

    public:
        Client();

        ~Client();

        bytes::binary init(int32_t g, const bytes::binary& p, const bytes::binary& r, const bytes::binary& g_a_hash);

        AuthParams confirmConnection(const bytes::binary& p, const bytes::binary& g_a_or_b, const uint64_t& fingerprint) const;

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
    };
}
