//
// Created by Laky64 on 21/08/2023.
//

#include "call_payload.hpp"

namespace ntgcalls {
    CallPayload::CallPayload(const wrtc::Description &desc, bool isGroup): isGroup(isGroup) {
        const auto [fingerprint, hash, setup, pwd, ufrag, audioSource, source_groups] = wrtc::SdpBuilder::parseSdp(desc.getSdp());
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
        if (isGroup) {
            jsonRes["ssrc"] = audioSource;
            if (!sourceGroups.empty()){
                jsonRes["ssrc-groups"] = {
                    {
                        {"semantics", "FID"},
                        {"sources", sourceGroups}
                    }
                };
            }
        } else {
            jsonRes["@type"] = "InitialSetup";
            jsonRes["audio"] = {
                {"ssrc", audioSource}
            };
            if (!sourceGroups.empty()){
                jsonRes["video"]["ssrcGroups"] = {
                    {
                        {"semantics", "FID"},
                        {"sources", sourceGroups}
                    }
                };
            }
        }
        return to_string(jsonRes);
    }

    CallPayload::operator bytes::binary() const {
        return bytes::binary(static_cast<std::string>(*this));
    }
} // ntgcalls