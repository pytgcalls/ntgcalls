//
// Created by Laky64 on 16/03/2024.
//

#include "signaling.hpp"

#include "signaling_v2.hpp"

namespace ntgcalls {
    std::unique_ptr<SignalingInterface> Signaling::Create(
        const std::vector<std::string> &versions,
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread,
        const EncryptionKey &key,
        const std::function<void(const bytes::binary&)>& onEmitData,
        const std::function<void(const std::optional<bytes::binary>&)>& onSignalData
    ) {
        if (versions.empty()) {
            RTC_LOG(LS_ERROR) << "No versions provided";
            throw SignalingError("No versions provided");
        }
        const auto bestVersion = bestMatch(versions);
        if (!bestVersion.empty()) {
            const auto sigVersion = signalingVersion(bestVersion);
            if (sigVersion == ProtocolVersion::V1) {
                RTC_LOG(LS_ERROR) << "Signaling V1 is not supported";
                throw SignalingUnsupported("Signaling V1 is not supported");
            }
            if (sigVersion & ProtocolVersion::V2) {
                RTC_LOG(LS_INFO) << "Using signaling V2 for version " << bestVersion;
                return std::make_unique<SignalingV2>(networkThread, signalingThread, key, onEmitData, onSignalData, sigVersion & ProtocolVersion::V2Full);
            }
        }
        throw SignalingUnsupported("Unsupported " + bestVersion + " protocol version");
    }

    std::vector<std::string> Signaling::SupportedVersions() {
        return {
            defaultVersion,
        };
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

    std::string Signaling::bestMatch(std::vector<std::string> versions) {
        std::ranges::sort(versions, [](const std::string& a, const std::string& b) {
            return version_to_tuple(b) < version_to_tuple(a);
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
        return "";
    }

    std::tuple<int, int, int> Signaling::version_to_tuple(const std::string& version) {
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

    int operator&(const Signaling::ProtocolVersion lhs, const Signaling::ProtocolVersion rhs) {
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }
} // ntgcalls