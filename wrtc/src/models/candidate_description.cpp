//
// Created by Laky64 on 10/09/2024.
//

#include <wrtc/models/candidate_description.hpp>

namespace wrtc {
    bool CandidateDescription::operator==(CandidateDescription const& rhs) const {
        if (protocol != rhs.protocol) {
            return false;
        }
        if (type != rhs.type) {
            return false;
        }
        if (address != rhs.address) {
            return false;
        }

        return true;
    }
} // wrtc