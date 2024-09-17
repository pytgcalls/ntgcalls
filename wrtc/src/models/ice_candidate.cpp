//
// Created by Laky64 on 15/03/2024.
//

#include <wrtc/models/ice_candidate.hpp>

namespace wrtc {
    IceCandidate::IceCandidate(std::string mid, const int mLine, std::string sdp): mid(std::move(mid)), mLine(mLine), sdp(std::move(sdp)) {}

    IceCandidate::IceCandidate(const webrtc::IceCandidateInterface* candidate){
        candidate->ToString(&sdp);
        mid = candidate->sdp_mid();
        mLine = candidate->sdp_mline_index();
    }

} // wrtc