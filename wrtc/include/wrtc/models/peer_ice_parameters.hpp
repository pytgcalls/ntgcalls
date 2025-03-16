//
// Created by Laky64 on 29/03/2024.
//

#pragma once
#include <string>
#include <utility>

namespace wrtc {

    struct PeerIceParameters {
        std::string ufrag;
        std::string pwd;
        bool supportsRenomination = false;

        PeerIceParameters() = default;

        PeerIceParameters(
            std::string ufrag,
            std::string pwd,
            const bool supportsRenomination
        ): ufrag(std::move(ufrag)), pwd(std::move(pwd)), supportsRenomination(supportsRenomination) {}
    };

} // wrtc
