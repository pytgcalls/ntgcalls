//
// Created by Laky64 on 10/09/2024.
//

#pragma once

#include <string>

namespace wrtc {
    struct RouteDescription {
        explicit RouteDescription(std::string localDescription, std::string remoteDescription);

        std::string localDescription;
        std::string remoteDescription;

        bool operator==(RouteDescription const &rhs) const;

        bool operator!=(const RouteDescription& rhs) const;
    };
} // wrtc
