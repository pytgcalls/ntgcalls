//
// Created by Lauren on 10/09/24.
//

#include <wrtc/models/route_description.hpp>

namespace wrtc::models {
    RouteDescription::RouteDescription(std::string local_description, std::string remote_description):
    local_description(std::move(local_description)),
    remote_description(std::move(remote_description)) {}

    bool RouteDescription::operator==(RouteDescription const& rhs) const {
        if (local_description != rhs.local_description) {
            return false;
        }
        if (remote_description != rhs.remote_description) {
            return false;
        }

        return true;
    }

    bool RouteDescription::operator!=(const RouteDescription& rhs) const {
        return !(*this == rhs);
    }
} // wrtc::models
