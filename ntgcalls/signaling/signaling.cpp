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
        const EncryptionKey &key,
        const std::function<void(const bytes::binary&)>& onEmitData,
        const std::function<void(const std::optional<bytes::binary>&)>& onSignalData
    ) {
        const auto version = signalingVersion(versions);
        if (version == ProtocolVersion::V1) {
            return std::make_shared<SignalingV1>(networkThread, signalingThread, key, onEmitData, onSignalData);
        }
        if (version & ProtocolVersion::V2) {
            return std::make_shared<SignalingV2>(networkThread, signalingThread, key, onEmitData, onSignalData, version & ProtocolVersion::V2Full);
        }
        throw ConnectionError("Unsupported version");
    }

    Signaling::ProtocolVersion Signaling::signalingVersion(const std::vector<std::string>& versions) {
        if (versions.empty()) {
            throw ConnectionError("No versions provided");
        }
        const auto bestVersion = std::ranges::find(versions, defaultVersion) != versions.end() ? defaultVersion : versions[0];
        if (bestVersion == "10.0.0") {
            return ProtocolVersion::V1;
        }
        if (bestVersion == "11.0.0") {
            return ProtocolVersion::V2Full;
        }
        return ProtocolVersion::Unknown;
    }

    int operator&(const Signaling::ProtocolVersion lhs, const Signaling::ProtocolVersion rhs) {
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }
} // ntgcalls