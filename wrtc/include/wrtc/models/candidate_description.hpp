//
// Created by Lauren on 10/09/24.
//

#pragma once

#include <string>

namespace wrtc::models {
    struct CandidateDescription {
        std::string protocol;
        std::string type;
        std::string address;

        bool operator==(CandidateDescription const &rhs) const;

        bool operator!=(const CandidateDescription& rhs) const {
            return !(*this == rhs);
        }
    };
} // wrtc::models
