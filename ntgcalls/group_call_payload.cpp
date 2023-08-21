//
// Created by Laky64 on 21/08/2023.
//

#include "group_call_payload.hpp"

namespace ntgcalls {
    GroupCallPayload::GroupCallPayload(wrtc::Description &desc) {
        const auto sdp = wrtc::SdpBuilder::parseSdp(desc.getSdp());
        ufrag = sdp.ufrag;
        pwd = sdp.pwd;
        fingerprint = sdp.fingerprint;
        hash = sdp.hash;
        setup = sdp.setup;
        audioSource = sdp.audioSource;
        for (auto &ssrc : sdp.source_groups) {
            sourceGroups.push_back(ssrc);
        }
    }

    GroupCallPayload::operator std::string() const {
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
            {"ssrc", audioSource},
        };
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