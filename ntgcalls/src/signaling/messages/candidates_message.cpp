//
// Created by Lauren on 30/03/24.
//

#include <ntgcalls/signaling/messages/candidates_message.hpp>

namespace ntgcalls::signaling::messages {
    bytes::binary CandidatesMessage::serialize() const {
        json res = {
            {"@type", "Candidates"},
        };
        auto ice_candidates_json = json::array();
        for (const auto& [sdpString, sdpMid, sdpMLineIndex] : ice_candidates) {
            ice_candidates_json.push_back(json{
                {"sdpString", sdpString},
                {"sdpMid", sdpMid},
                {"sdpMLineIndex", sdpMLineIndex},
            });
        }
        res["candidates"] = ice_candidates_json;
        return bytes::make_binary(res.dump());
    }

    std::unique_ptr<CandidatesMessage> CandidatesMessage::deserialize(const bytes::binary& data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<CandidatesMessage>();
        for (const auto& ice_candidate : j["candidates"]) {
            std::string sdp_mid;
            if (ice_candidate.contains("sdpMid")) {
                sdp_mid = ice_candidate["sdpMid"].get<std::string>();
            }
            // ReSharper disable once CppDFAUnreadVariable
            int sdp_m_line_index = 0;
            if (ice_candidate.contains("sdpMLineIndex")) {
                // ReSharper disable once CppDFAUnusedValue
                sdp_m_line_index = ice_candidate["sdpMLineIndex"].get<int>();
            }
            message->ice_candidates.push_back(IceCandidate{
                ice_candidate["sdpString"],
                std::move(sdp_mid),
                sdp_m_line_index,
            });
        }
        return std::move(message);
    }
} // ntgcalls::signaling::messages
