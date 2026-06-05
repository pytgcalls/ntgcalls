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
        for (const auto& [sdpString, sdpMid, sdpMLineIndex] : iceCandidates) {
            iceCandidatesJson.push_back(json{
                {"sdpString", sdpString},
                {"sdpMid", sdpMid},
                {"sdpMLineIndex", sdpMLineIndex}
            });
        }
        res["candidates"] = iceCandidatesJson;
        return bytes::make_binary(res.dump());
    }

    std::unique_ptr<CandidatesMessage> CandidatesMessage::deserialize(const bytes::binary &data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<CandidatesMessage>();
        for (const auto& iceCandidate : j["candidates"]) {
            std::string sdpMid = "";
            if (iceCandidate.contains("sdpMid")) {
                sdpMid = iceCandidate["sdpMid"].get<std::string>();
            }
            // ReSharper disable once CppDFAUnreadVariable
            int sdpMLineIndex = 0;
            if (iceCandidate.contains("sdpMLineIndex")) {
                // ReSharper disable once CppDFAUnusedValue
                sdpMLineIndex = iceCandidate["sdpMLineIndex"].get<int>();
            }
            message->iceCandidates.push_back(IceCandidate{
                iceCandidate["sdpString"],
                std::move(sdpMid),
                sdpMLineIndex
            });
        }
        return std::move(message);
    }
} // signaling