//
// Created by Lauren on 30/03/24.
//

#pragma once
#include <string>
#include <vector>

#include <ntgcalls/signaling/messages/message.hpp>

namespace ntgcalls::signaling::messages {
    class CandidatesMessage final : public Message {
    public:
        struct IceCandidate {
            std::string sdp_string;
            std::string sdp_mid;
            int sdp_m_line_index = 0;
        };

        std::vector<IceCandidate> ice_candidates;

        [[nodiscard]] bytes::binary serialize() const override;

        static std::unique_ptr<CandidatesMessage> deserialize(const bytes::binary& data);
    };

} // ntgcalls::signaling::messages