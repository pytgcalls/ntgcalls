//
// Created by Laky64 on 10/09/2024.
//

#pragma once
#include <wrtc/models/candidate_description.hpp>

namespace wrtc {
    struct ConnectionDescription {
        CandidateDescription local;
        CandidateDescription remote;

        bool operator==(ConnectionDescription const &rhs) const;

        bool operator!=(const ConnectionDescription& rhs) const;
    };
} // wrtc
