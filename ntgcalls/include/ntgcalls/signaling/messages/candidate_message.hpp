//
// Created by Laky64 on 22/03/2024.
//

#pragma once
#include <ntgcalls/signaling/messages/message.hpp>

namespace signaling {

    class CandidateMessage final : public Message {
    public:
        std::string mid;
        int mLine;
        std::string sdp;

        [[nodiscard]] bytes::binary serialize() const override;

        static std::unique_ptr<CandidateMessage> deserialize(const bytes::binary& data);
    };

} // signaling
