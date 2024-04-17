//
// Created by Laky64 on 16/03/2024.
//

#pragma once

#include "signaling_interface.hpp"
#include "ntgcalls/exceptions.hpp"

namespace signaling {

    class Signaling {
    public:
        enum class Version{
            Unknown = 0,
            V1 = 1 << 0,
            V2 = 1 << 1,
            V2Full = 1 << 2
        };

        static std::unique_ptr<SignalingInterface> Create(
            Version version,
            rtc::Thread* networkThread,
            rtc::Thread* signalingThread,
            const EncryptionKey &key,
            const DataEmitter& onEmitData,
            const DataReceiver& onSignalData
        );

        static std::vector<std::string> SupportedVersions();

        static Version matchVersion(const std::vector<std::string> &versions);

    private:
#ifdef LEGACY_SUPPORT
        static constexpr char defaultVersion[] = "8.0.0";
#else
        static constexpr char defaultVersion[] = "11.0.0";
#endif

        static std::string bestMatch(std::vector<std::string> versions);

        static std::tuple<int, int, int> versionToTuple(const std::string& version);
    };

    inline bool operator&(const Signaling::Version lhs, Signaling::Version rhs) {
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }
} // signaling
