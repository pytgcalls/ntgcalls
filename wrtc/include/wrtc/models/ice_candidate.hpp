//
// Created by Lauren on 15/03/24.
//

#pragma once
#include <string>
#include <api/jsep_ice_candidate.h>

namespace wrtc::models {

    class IceCandidate {
    public:
        std::string mid;
        int m_line;
        std::string sdp;

        IceCandidate(std::string mid, int m_line, std::string sdp);

        explicit IceCandidate(const webrtc::IceCandidateInterface* candidate);
    };

} // wrtc::models
