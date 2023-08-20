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
#include "join_voice_call_params.hpp"

namespace ntgcalls {
    using nlohmann::json;

    class Client {
    public:
        Client() = default;

        std::string createCall(std::string audioPath);

        void setRemoteCallParams(const std::string& jsonData);

    private:
        std::shared_ptr<wrtc::PeerConnection> connection;
        wrtc::SSRC audioSource = 0;
        std::vector<wrtc::SSRC> sourceGroups = {};
        std::shared_ptr<Stream> stream;

        JoinVoiceCallParams init();
    };
}