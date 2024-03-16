//
// Created by Laky64 on 16/03/2024.
//

#include "signaling.hpp"

#include "signaling_v1.hpp"
#include "signaling_v2.hpp"

namespace ntgcalls {
    std::shared_ptr<SignalingInterface> Signaling::Create(
        const std::vector<std::string> &versions,
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread,
        bool isOutGoing,
        const Key &key,
        const std::function<void(const bytes::binary&)>& onEmitData,
        const std::function<void(const std::optional<bytes::binary>&)>& onSignalData
    ) {
        const auto version = SignalingInterface::signalingVersion(versions);
        if (version == SignalingInterface::ProtocolVersion::V1) {
            return std::make_shared<SignalingV1>(networkThread, signalingThread, isOutGoing, key, onEmitData, onSignalData);
        }
        if (version == SignalingInterface::ProtocolVersion::V2) {
            return std::make_shared<SignalingV2>(networkThread, signalingThread, isOutGoing, key, onEmitData, onSignalData);
        }
        throw ConnectionError("Unsupported version");
    }
} // ntgcalls