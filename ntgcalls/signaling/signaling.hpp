//
// Created by Laky64 on 16/03/2024.
//

#pragma once

#include "signaling_interface.hpp"
#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {

    class Signaling {
    public:
        enum class ProtocolVersion{
            Unknown = 1 << 0,
            V1 = 1 << 1,
            V2 = 1 << 2,
            V2Full = V2 | 1 << 3
        };

        static std::shared_ptr<SignalingInterface> Create(
            const std::vector<std::string> &versions,
            rtc::Thread* networkThread,
            rtc::Thread* signalingThread,
            const EncryptionKey &key,
            const std::function<void(const bytes::binary&)>& onEmitData,
            const std::function<void(const std::optional<bytes::binary>&)>& onSignalData
        );

        static std::vector<std::string> SupportedVersions();

    private:
        static constexpr std::string defaultVersion = "11.0.0";

        static ProtocolVersion signalingVersion(const std::string& version);

        static std::string bestMatch(std::vector<std::string> versions);

        static std::tuple<int, int, int> version_to_tuple(const std::string& version);
    };

    inline int operator&(Signaling::ProtocolVersion lhs, Signaling::ProtocolVersion rh);
} // ntgcalls
