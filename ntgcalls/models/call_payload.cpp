//
// Created by Laky64 on 21/08/2023.
//

#include "call_payload.hpp"

namespace ntgcalls {
    CallPayload::CallPayload(const wrtc::Description &desc) {
        const auto [fingerprint, hash, setup, pwd, ufrag, audioSource, source_groups] = wrtc::SdpBuilder::parseSdp(desc.sdp());
        this->ufrag = ufrag;
        this->pwd = pwd;
        this->fingerprint = fingerprint;
        this->hash = hash;
        this->setup = setup;
        this->audioSource = static_cast<wrtc::TgSSRC>(audioSource);
        for (auto &ssrc : source_groups) {
            this->sourceGroups.push_back(static_cast<wrtc::TgSSRC>(ssrc));
        }
    }

    CallPayload::operator std::string() const {
        json jsonRes = {
            {"ufrag", ufrag},
            {"pwd", pwd},
            {"fingerprints", {
                    {
                        {"hash", hash},
                        {"setup", "active"},
                        {"fingerprint", fingerprint}
                    }
            }},
        };
        jsonRes["ssrc"] = audioSource;
        if (!sourceGroups.empty()){
            jsonRes["ssrc-groups"] = {
                {
                    {"semantics", "FID"},
                    {"sources", sourceGroups}
                }
            };
        }
        return to_string(jsonRes);
    }
} // ntgcalls