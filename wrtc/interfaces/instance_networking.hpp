//
// Created by Laky64 on 29/03/2024.
//

#pragma once
#include <string>
#include <pc/sctp_data_channel.h>

namespace wrtc {

    class InstanceNetworking {
    public:
        struct ConnectionDescription {
            struct CandidateDescription {
                std::string protocol;
                std::string type;
                std::string address;

                bool operator==(CandidateDescription const &rhs) const {
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

                bool operator!=(const CandidateDescription& rhs) const {
                    return !(*this == rhs);
                }
            };

            CandidateDescription local;
            CandidateDescription remote;

            bool operator==(ConnectionDescription const &rhs) const {
                if (local != rhs.local) {
                    return false;
                }
                if (remote != rhs.remote) {
                    return false;
                }

                return true;
            }

            bool operator!=(const ConnectionDescription& rhs) const {
                return !(*this == rhs);
            }
        };

        struct RouteDescription {
            explicit RouteDescription(std::string localDescription_, std::string remoteDescription_);

            std::string localDescription;
            std::string remoteDescription;

            bool operator==(RouteDescription const &rhs) const {
                if (localDescription != rhs.localDescription) {
                    return false;
                }
                if (remoteDescription != rhs.remoteDescription) {
                    return false;
                }

                return true;
            }

            bool operator!=(const RouteDescription& rhs) const {
                return !(*this == rhs);
            }
        };

        struct State {
            bool isReadyToSendData = false;
            bool isFailed = false;
            absl::optional<RouteDescription> route;
            absl::optional<ConnectionDescription> connection;
        };

        static ConnectionDescription::CandidateDescription connectionDescriptionFromCandidate(const cricket::Candidate &candidate);
    };
} // wrtc