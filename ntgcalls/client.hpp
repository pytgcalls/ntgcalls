//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <string>
#include <optional>
#include <wrtc/wrtc.hpp>

#include "stream.hpp"
#include "exceptions.hpp"
#include "io/file_reader.hpp"
#include "models/media_description.hpp"
#include "models/group_call_payload.hpp"

namespace ntgcalls {
    using nlohmann::json;

    class Client {
    private:
        std::shared_ptr<wrtc::PeerConnection> connection;
        wrtc::SSRC audioSource = 0;
        std::vector<wrtc::SSRC> sourceGroups = {};
        std::shared_ptr<Stream> stream;

        GroupCallPayload init();

    public:
        Client();

        ~Client();

        std::string init(MediaDescription config);

        void connect(const std::string& jsonData);

        void changeStream(MediaDescription config);

        bool pause();

        bool resume();

        bool mute();

        bool unmute();

        void stop();

        uint64_t time();

        MediaState getState();

        Stream::Status status();

        void onUpgrade(std::function<void(MediaState)> callback);

        void onStreamEnd(std::function<void(Stream::Type)> callback);
    };
}