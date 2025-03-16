//
// Created by Laky64 on 15/03/2024.
//

#pragma once
#include <string>
#include <api/jsep_ice_candidate.h>

namespace wrtc {

    class IceCandidate {
    public:
        std::string mid;
        int mLine;
        std::string sdp;

        IceCandidate(std::string mid, int mLine, std::string sdp);

        explicit IceCandidate(const webrtc::IceCandidateInterface *candidate);
    };

} // wrtc
