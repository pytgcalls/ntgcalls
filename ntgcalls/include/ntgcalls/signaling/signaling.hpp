//
// Created by Lauren on 16/03/24.
//

#pragma once

#include <api/environment/environment.h>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/signaling/signaling_interface.hpp>

namespace ntgcalls::signaling {

    class Signaling {
        static std::string best_match(std::vector<std::string> versions);

    public:
        enum class Version {
            Unknown = 0,
            V1 = 1 << 0,
            V2 = 1 << 1,
            V2Full = 1 << 2,
            V3 = 1 << 3,
        };

        static std::shared_ptr<SignalingInterface> create(
            Version version,
            wrtc::utils::SafeThread& network_thread,
            wrtc::utils::SafeThread& signaling_thread,
            const webrtc::Environment&,
            const crypto::EncryptionKey& key,
            const DataEmitter& on_emit_data,
            const DataReceiver& on_signal_data
        );

        static std::vector<std::string> supported_versions();

        static Version match_version(const std::vector<std::string>& versions);
    };

    inline bool operator&(const Signaling::Version lhs, Signaling::Version rhs) {
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }
} // ntgcalls::signaling
