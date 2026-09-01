//
// Created by Lauren on 10/09/24.
//

#include <wrtc/models/connection_description.hpp>

namespace wrtc::models {
    bool ConnectionDescription::operator==(ConnectionDescription const& rhs) const {
        if (local != rhs.local) {
            return false;
        }
        if (remote != rhs.remote) {
            return false;
        }
        return true;
    }

    bool ConnectionDescription::operator!=(const ConnectionDescription& rhs) const {
        return !(*this == rhs);
    }
} // wrtc::models
