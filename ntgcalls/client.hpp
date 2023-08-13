//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include <wrtc/wrtc.hpp>

#include "io/file_reader.hpp"
#include "utils/time.hpp"
#include "exceptions.hpp"
#include "stream.hpp"

namespace ntgcalls {
    using nlohmann::json;

    struct JoinVoiceCallParams {
        std::string ufrag;
        std::string pwd;
        std::string hash;
        std::string setup;
        std::string fingerprint;
    };

    class Client {
    private:
        std::shared_ptr<wrtc::PeerConnection> connection;
        wrtc::SSRC audioSource = 0;
        std::vector<wrtc::SSRC> sourceGroups = {};
        std::shared_ptr<Stream> stream;

        std::optional<JoinVoiceCallParams> init();

    public:
        std::string createCall(std::string audioPath);

        void setRemoteCallParams(const std::string& jsonData);
    };
}