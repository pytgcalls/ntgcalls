//
// Created by Laky64 on 16/03/2024.
//

#include <ntgcalls/signaling/signaling.hpp>

#include <ntgcalls/signaling/external_signaling_connection.hpp>
#include <ntgcalls/signaling/signaling_sctp_connection.hpp>
#include <ntgcalls/utils/version_parser.hpp>


namespace signaling {
    std::shared_ptr<SignalingInterface> Signaling::Create(
        const Version version,
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread,
        const webrtc::Environment& env,
        const EncryptionKey &key,
        const DataEmitter& onEmitData,
        const DataReceiver& onSignalData
    ) {
        if (version == Version::V1) {
            RTC_LOG(LS_ERROR) << "Signaling V1 is not supported";
            throw ntgcalls::SignalingUnsupported("Signaling V1 is not supported");
        }
        if (version & Version::V2) {
            RTC_LOG(LS_INFO) << "Using signaling V2 Legacy";
            return std::make_shared<ExternalSignalingConnection>(networkThread, signalingThread, key, onEmitData, onSignalData);
        }
        if (version & Version::V2Full) {
            RTC_LOG(LS_INFO) << "Using signaling V2 Full";
            return std::make_shared<SignalingSctpConnection>(networkThread, signalingThread, env, key, onEmitData, onSignalData, true);
        }
        throw ntgcalls::SignalingUnsupported("Unsupported protocol version");
    }

    std::vector<std::string> Signaling::SupportedVersions() {
        return {
            "8.0.0",
            "9.0.0",
            "11.0.0",
        };
    }

    Signaling::Version Signaling::matchVersion(const std::vector<std::string> &versions) {
        const auto version = bestMatch(versions);
        RTC_LOG(LS_INFO) << "Selected version: " << version;
        if (version == "8.0.0" || version == "9.0.0") {
            return Version::V2;
        }
        if (version == "10.0.0") {
            return Version::V1;
        }
        if (version == "11.0.0") {
            return Version::V2Full;
        }
        throw ntgcalls::SignalingUnsupported("Unsupported " + version + " protocol version");
    }

    std::string Signaling::bestMatch(std::vector<std::string> versions) {
        if (versions.empty()) {
            RTC_LOG(LS_ERROR) << "No versions provided";
            throw ntgcalls::SignalingError("No versions provided");
        }
        std::ranges::sort(versions, [](const std::string& a, const std::string& b) {
            return ntgcalls::VersionParser::Parse(b) < ntgcalls::VersionParser::Parse(a);
        });
        auto supported = SupportedVersions();
        for (const auto& version : versions) {
            if (std::ranges::find(supported, defaultVersion) != supported.end()) {
                return defaultVersion;
            }
            if (std::ranges::find(supported, version) != supported.end()) {
                return version;
            }
        }
        throw ntgcalls::SignalingUnsupported("No supported version found");
    }
} // signaling