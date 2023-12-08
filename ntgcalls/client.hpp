//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <string>
#include <wrtc/wrtc.hpp>

#include "stream.hpp"
#include "models/media_description.hpp"
#include "models/group_call_payload.hpp"

namespace ntgcalls {
    using nlohmann::json;

    class Client {
        std::shared_ptr<wrtc::PeerConnection> connection;
        wrtc::SSRC audioSource = 0;
        std::vector<wrtc::SSRC> sourceGroups = {};
        std::shared_ptr<Stream> stream;

        GroupCallPayload init();

    public:
        Client();

        ~Client();

        std::string init(const MediaDescription& config);

        void connect(const std::string& jsonData) const;

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
    };
}