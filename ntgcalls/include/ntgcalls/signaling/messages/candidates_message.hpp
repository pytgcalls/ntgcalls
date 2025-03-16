//
// Created by Laky64 on 30/03/2024.
//

#pragma once
#include <string>
#include <vector>

#include <ntgcalls/signaling/messages/message.hpp>

namespace signaling {
    class CandidatesMessage final : public Message {
    public:
        struct IceCandidate {
            std::string sdpString;
        };

        std::vector<IceCandidate> iceCandidates;

        [[nodiscard]] bytes::binary serialize() const override;

        static std::unique_ptr<CandidatesMessage> deserialize(const bytes::binary& data);
    };

} // signaling