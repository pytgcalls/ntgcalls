//
// Created by Laky64 on 29/03/2024.
//

#include "instance_networking.hpp"

namespace wrtc {
    InstanceNetworking::RouteDescription::RouteDescription(std::string localDescription_,
        std::string remoteDescription_):
        localDescription(std::move(localDescription_)),
        remoteDescription(std::move(remoteDescription_))
    {}

    InstanceNetworking::ConnectionDescription::CandidateDescription InstanceNetworking::connectionDescriptionFromCandidate(const cricket::Candidate &candidate) {
        ConnectionDescription::CandidateDescription result;
        result.type = candidate.type_name();
        result.protocol = candidate.protocol();
        result.address = candidate.address().ToString();
        return result;
    }
} // wrtc