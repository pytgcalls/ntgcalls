//
// Created by Laky64 on 30/03/2024.
//

#include <ntgcalls/signaling/messages/candidates_message.hpp>

namespace signaling {
    bytes::binary CandidatesMessage::serialize() const {
        json res = {
            {"@type", "Candidates"},
        };
        auto iceCandidatesJson = json::array();
        for (const auto& [sdpString] : iceCandidates) {
            iceCandidatesJson.push_back(json{
                {"sdpString", sdpString},
            });
        }
        res["candidates"] = iceCandidatesJson;
        return bytes::make_binary(to_string(res));
    }

    std::unique_ptr<CandidatesMessage> CandidatesMessage::deserialize(const bytes::binary &data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<CandidatesMessage>();
        for (const auto& iceCandidate : j["candidates"]) {
            message->iceCandidates.push_back(IceCandidate{iceCandidate["sdpString"]});
        }
        return std::move(message);
    }
} // signaling