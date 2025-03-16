//
// Created by Laky64 on 16/03/2024.
//

#pragma once

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/signaling/signaling_interface.hpp>

namespace signaling {

    class Signaling {
    public:
        enum class Version{
            Unknown = 0,
            V1 = 1 << 0,
            V2 = 1 << 1,
            V2Full = 1 << 2
        };

        static std::shared_ptr<SignalingInterface> Create(
            Version version,
            rtc::Thread* networkThread,
            rtc::Thread* signalingThread,
            const webrtc::Environment& env,
            const EncryptionKey &key,
            const DataEmitter& onEmitData,
            const DataReceiver& onSignalData
        );

        static std::vector<std::string> SupportedVersions();

        static Version matchVersion(const std::vector<std::string> &versions);

    private:
        static constexpr char defaultVersion[] = "8.0.0";

        static std::string bestMatch(std::vector<std::string> versions);
    };

    inline bool operator&(const Signaling::Version lhs, Signaling::Version rhs) {
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }
} // signaling
