//
// Created by Lauren on 29/03/24.
//

#pragma once
#include <string>
#include <utility>

namespace wrtc::models {

    struct PeerIceParameters {
        std::string ufrag;
        std::string pwd;
        bool supports_renomination = false;

        PeerIceParameters() = default;

        PeerIceParameters(
            std::string ufrag,
            std::string pwd,
            const bool supports_renomination
        ): ufrag(std::move(ufrag)), pwd(std::move(pwd)), supports_renomination(supports_renomination) {}
    };

} // wrtc::models
