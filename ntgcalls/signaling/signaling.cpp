//
// Created by Laky64 on 16/03/2024.
//

#include "signaling.hpp"

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
        if (versions.empty()) {
            throw SignalingError("No versions provided");
        }
        const auto bestVersion = std::ranges::find(versions, defaultVersion) != versions.end() ? defaultVersion : versions[0];
        const auto sigVersion = signalingVersion(bestVersion);
        if (sigVersion == ProtocolVersion::V1) {
            throw SignalingUnsupported("Signaling V1 is not supported");
        }
        if (sigVersion & ProtocolVersion::V2) {
            return std::make_shared<SignalingV2>(networkThread, signalingThread, key, onEmitData, onSignalData, sigVersion & ProtocolVersion::V2Full);
        }
        throw SignalingUnsupported("Unsupported " + bestVersion + " protocol version");
    }

    Signaling::ProtocolVersion Signaling::signalingVersion(const std::string& version) {
        if (version == "10.0.0") {
            return ProtocolVersion::V1;
        }
        if (version == "11.0.0") {
            return ProtocolVersion::V2Full;
        }
        return ProtocolVersion::Unknown;
    }

    int operator&(const Signaling::ProtocolVersion lhs, const Signaling::ProtocolVersion rhs) {
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }
} // ntgcalls