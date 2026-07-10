//
// Created by Lauren on 10/09/24.
//

#pragma once
#include <wrtc/models/candidate_description.hpp>

namespace wrtc::models {
    struct ConnectionDescription {
        CandidateDescription local;
        CandidateDescription remote;

        bool operator==(ConnectionDescription const &rhs) const;

        bool operator!=(const ConnectionDescription& rhs) const;
    };
} // wrtc::models
