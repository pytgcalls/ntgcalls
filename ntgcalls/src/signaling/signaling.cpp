//
// Created by Lauren on 16/03/24.
//

#include <ntgcalls/signaling/external_signaling_connection.hpp>
#include <ntgcalls/signaling/signaling.hpp>
#include <ntgcalls/signaling/signaling_sctp_connection.hpp>
#include <ntgcalls/utils/version_parser.hpp>

namespace ntgcalls::signaling {
    std::shared_ptr<SignalingInterface> Signaling::create(
        const Version version,
        wrtc::utils::SafeThread& network_thread,
        wrtc::utils::SafeThread& signaling_thread,
        const webrtc::Environment& env,
        const crypto::EncryptionKey &key,
        const DataEmitter& on_emit_data,
        const DataReceiver& on_signal_data
    ) {
        std::shared_ptr<SignalingInterface> signaling;
        if (version & Version::V3) {
            RTC_LOG(LS_VERBOSE) << "Using signaling V3";
            signaling = std::make_shared<SignalingSctpConnection>(network_thread, signaling_thread, env, key, on_emit_data, on_signal_data, true);
        }
        if (!signaling) {
            RTC_LOG(LS_VERBOSE) << "Using signaling V2 Legacy";
            signaling = std::make_shared<ExternalSignalingConnection>(network_thread, signaling_thread, key, on_emit_data, on_signal_data);
        }
        signaling->init();
        return signaling;
    }

    std::vector<std::string> Signaling::supported_versions() {
        return {
            "8.0.0",
            "9.0.0",
            "12.0.0",
            "13.0.0",
        };
    }

    Signaling::Version Signaling::match_version(const std::vector<std::string> &versions) {
        const auto version = best_match(versions);
        RTC_LOG(LS_INFO) << "Selected version: " << version;
        if (version == "8.0.0" || version == "9.0.0") {
            return Version::V2;
        }
        if (version == "12.0.0" || version == "13.0.0") {
            return Version::V3;
        }
        throw SignalingUnsupported("Unsupported " + version + " protocol version");
    }

    std::string Signaling::best_match(std::vector<std::string> versions) {
        if (versions.empty()) {
            RTC_LOG(LS_ERROR) << "No versions provided";
            throw SignalingError("No versions provided");
        }
        std::ranges::sort(versions, [](const std::string& a, const std::string& b) {
            return utils::VersionParser::Parse(b) < utils::VersionParser::Parse(a);
        });
        auto supported = supported_versions();
        for (const auto& version : versions) {
            if (std::ranges::find(supported, version) != supported.end()) {
                return version;
            }
        }
        throw SignalingUnsupported("No supported version found");
    }
} // ntgcalls::signaling