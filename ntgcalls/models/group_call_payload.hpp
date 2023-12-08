//
// Created by Laky64 on 21/08/2023.
//

#pragma once

#include <wrtc/wrtc.hpp>
#include <nlohmann/json.hpp>


namespace ntgcalls {
    using nlohmann::json;

    class GroupCallPayload {
    public:
        std::string ufrag;
        std::string pwd;
        std::string hash;
        std::string setup;
        std::string fingerprint;
        wrtc::TgSSRC audioSource;
        std::vector<wrtc::TgSSRC> sourceGroups;

        explicit GroupCallPayload(wrtc::Description &desc);

        explicit operator std::string() const;
    };

} // ntgcalls

