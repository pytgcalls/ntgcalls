//
// Created by Laky64 on 10/09/2024.
//

#include <wrtc/models/route_description.hpp>

namespace wrtc {
    RouteDescription::RouteDescription(std::string localDescription, std::string remoteDescription):
        localDescription(std::move(localDescription)),
        remoteDescription(std::move(remoteDescription)){}


    bool RouteDescription::operator==(RouteDescription const& rhs) const {
        if (localDescription != rhs.localDescription) {
            return false;
        }
        if (remoteDescription != rhs.remoteDescription) {
            return false;
        }

        return true;
    }

    bool RouteDescription::operator!=(const RouteDescription& rhs) const {
        return !(*this == rhs);
    }
} // wrtc