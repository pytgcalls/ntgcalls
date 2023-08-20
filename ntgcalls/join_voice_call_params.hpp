//
// Created by Laky64 on 20/08/2023.
//

#pragma once

#include <nlohmann/json.hpp>
#include <wrtc/wrtc.hpp>

namespace ntgcalls {
    using nlohmann::json;

    struct JoinVoiceCallParams {
        std::string ufrag;
        std::string pwd;
        std::string hash;
        std::string setup;
        std::string fingerprint;
        wrtc::SSRC audioSource;
        std::vector<wrtc::SSRC> sourceGroups;

        operator std::string() const;
    };

} // ntgcalls
