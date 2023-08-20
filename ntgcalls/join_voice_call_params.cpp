//
// Created by Laky64 on 20/08/2023.
//

#include "join_voice_call_params.hpp"

namespace ntgcalls {

    JoinVoiceCallParams::operator std::string() const {
        json jsonRes = {
            {"ufrag", ufrag},
            {"pwd", pwd},
            {"fingerprints", {
                {
                    {"hash", hash},
                    {"setup", setup},
                    {"fingerprint", fingerprint}
                }
            }},
            {"ssrc", wrtc::SdpBuilder::toTelegramSSRC(audioSource)},
        };
        if (!sourceGroups.empty()){
            jsonRes["ssrc-groups"] = {
                {
                    {"semantics", "FID"},
                    {"sources", wrtc::SdpBuilder::toTelegramSSRC(sourceGroups)}
                }
            };
        }
        return to_string(jsonRes);
    }

} // ntgcalls