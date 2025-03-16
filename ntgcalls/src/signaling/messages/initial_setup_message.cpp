//
// Created by Laky64 on 28/03/2024.
//

#include <ntgcalls/signaling/messages/initial_setup_message.hpp>

namespace signaling {
    bytes::binary InitialSetupMessage::serialize() const {
        json res = {
            {"@type", "InitialSetup"},
            {"ufrag", ufrag},
            {"pwd", pwd},
            {"renomination", supportsRenomination},
        };
        json fingerprintsJson = json::array();
        for (const auto& [hash, setup, fingerprint] : fingerprints) {
            fingerprintsJson.push_back(json{
                {"hash", hash},
                {"setup", setup},
                {"fingerprint", fingerprint},
            });
        }
        res["fingerprints"] = fingerprintsJson;
        return bytes::make_binary(to_string(res));
    }

    std::unique_ptr<InitialSetupMessage> InitialSetupMessage::deserialize(const bytes::binary& data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<InitialSetupMessage>();
        message->ufrag = j["ufrag"];
        message->pwd = j["pwd"];
        message->supportsRenomination = j["renomination"];
        for (const auto& fingerprint : j["fingerprints"]) {
            message->fingerprints.push_back({
                fingerprint["hash"],
                fingerprint["setup"],
                fingerprint["fingerprint"],
            });
        }
        return std::move(message);
    }
} // signaling