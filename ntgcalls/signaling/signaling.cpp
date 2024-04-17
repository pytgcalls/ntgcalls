//
// Created by Laky64 on 16/03/2024.
//

#include "signaling.hpp"

#include "signaling_sctp_connection.hpp"


namespace signaling {
    std::unique_ptr<SignalingInterface> Signaling::Create(
        const Version version,
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread,
        const EncryptionKey &key,
        const DataEmitter& onEmitData,
        const DataReceiver& onSignalData
    ) {
        if (version == Version::V1) {
            RTC_LOG(LS_ERROR) << "Signaling V1 is not supported";
            throw ntgcalls::SignalingUnsupported("Signaling V1 is not supported");
        }
        if (version & Version::V2Full) {
            RTC_LOG(LS_INFO) << "Using signaling V2 Full";
            return std::make_unique<SignalingSctpConnection>(networkThread, signalingThread, key, onEmitData, onSignalData, true);
        }
        throw ntgcalls::SignalingUnsupported("Unsupported protocol version");
    }

    std::vector<std::string> Signaling::SupportedVersions() {
        return {
            "11.0.0",
        };
    }

    Signaling::Version Signaling::matchVersion(const std::vector<std::string> &versions) {
        const auto version = bestMatch(versions);
        RTC_LOG(LS_INFO) << "Selected version: " << version;
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
            return versionToTuple(b) < versionToTuple(a);
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

    std::tuple<int, int, int> Signaling::versionToTuple(const std::string& version) {
        try {
            std::vector<std::string> parts;
            std::istringstream stream(version);
            std::string part;
            while (std::getline(stream, part, '.')) {
                parts.push_back(part);
            }
            if (parts.size() != 3) {
                return std::make_tuple(0, 0, 0);
            }
            return std::make_tuple(std::stoi(parts[0]), std::stoi(parts[1]), std::stoi(parts[2]));
        } catch (const std::invalid_argument&) {
            return std::make_tuple(0, 0, 0);
        }
    }
} // signaling