//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <string>
#include <optional>
#include <wrtc/wrtc.hpp>

#include "stream.hpp"
#include "exceptions.hpp"
#include "utils/time.hpp"
#include "io/file_reader.hpp"
#include "group_call_payload.hpp"

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
        Client() = default;

        std::string init(StreamConfig config);

        void connect(const std::string& jsonData);

        void changeStream(StreamConfig config);

        void pause();

        void resume();

        void mute();

        void unmute();

        void stop();
    };
}