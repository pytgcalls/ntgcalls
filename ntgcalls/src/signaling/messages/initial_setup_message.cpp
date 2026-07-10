//
// Created by Lauren on 28/03/24.
//

#include <ntgcalls/signaling/messages/initial_setup_message.hpp>

namespace ntgcalls::signaling::messages {
    bytes::binary InitialSetupMessage::serialize() const {
        json res = {
            {"@type", "InitialSetup"},
            {"ufrag", ufrag},
            {"pwd", pwd},
            {"renomination", supports_renomination},
        };
        json fingerprints_json = json::array();
        for (const auto& [hash, setup, fingerprint] : fingerprints) {
            fingerprints_json.push_back(json{
                {"hash", hash},
                {"setup", setup},
                {"fingerprint", fingerprint},
            });
        }
        res["fingerprints"] = fingerprints_json;
        return bytes::make_binary(res.dump());
    }

    std::unique_ptr<InitialSetupMessage> InitialSetupMessage::deserialize(const bytes::binary& data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<InitialSetupMessage>();
        message->ufrag = j["ufrag"];
        message->pwd = j["pwd"];
        message->supports_renomination = j["renomination"];
        for (const auto& fingerprint : j["fingerprints"]) {
            message->fingerprints.push_back({
                fingerprint["hash"],
                fingerprint["setup"],
                fingerprint["fingerprint"],
            });
        }
        return std::move(message);
    }
} // ntgcalls::signaling::messages