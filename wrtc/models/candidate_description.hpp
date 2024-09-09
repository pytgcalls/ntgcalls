//
// Created by Laky64 on 10/09/2024.
//

#pragma once

#include <string>

namespace wrtc {
    struct CandidateDescription {
        std::string protocol;
        std::string type;
        std::string address;

        bool operator==(CandidateDescription const &rhs) const;

        bool operator!=(const CandidateDescription& rhs) const {
            return !(*this == rhs);
        }
    };
} // wrtc
