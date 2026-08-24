//
// Created by Lauren on 10/09/24.
//

#pragma once

#include <string>

namespace wrtc::models {
    struct RouteDescription {
        explicit RouteDescription(std::string local_description, std::string remote_description);

        std::string local_description;
        std::string remote_description;

        bool operator==(RouteDescription const& rhs) const;

        bool operator!=(const RouteDescription& rhs) const;
    };
} // wrtc::models
