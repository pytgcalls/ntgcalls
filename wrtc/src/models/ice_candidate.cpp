//
// Created by Lauren on 15/03/24.
//

#include <wrtc/models/ice_candidate.hpp>

namespace wrtc::models {
    IceCandidate::IceCandidate(std::string mid, const int m_line, std::string sdp): mid(std::move(mid)), m_line(m_line), sdp(std::move(sdp)) {}

    IceCandidate::IceCandidate(const webrtc::IceCandidateInterface* candidate){
        candidate->ToString(&sdp);
        mid = candidate->sdp_mid();
        m_line = candidate->sdp_mline_index();
    }

} // wrtc::models