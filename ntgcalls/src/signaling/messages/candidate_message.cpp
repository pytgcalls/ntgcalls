//
// Created by Laky64 on 22/03/2024.
//

#include <ntgcalls/signaling/messages/candidate_message.hpp>

namespace signaling {
    bytes::binary CandidateMessage::serialize() const {
        return bytes::make_binary(to_string(json{
            {"@type", "candidate"},
            {"mid", mid},
            {"mline", mLine},
            {"sdp", sdp},
        }));
    }

    std::unique_ptr<CandidateMessage> CandidateMessage::deserialize(const bytes::binary& data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<CandidateMessage>();
        message->mid = j["mid"];
        message->mLine = j["mline"];
        message->sdp = j["sdp"];
        return std::move(message);
    }
} // signaling