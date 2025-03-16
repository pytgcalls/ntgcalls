//
// Created by Laky64 on 10/09/2024.
//

#include <wrtc/models/connection_description.hpp>

namespace wrtc {
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
} // wrtc