//
// Created by Lauren on 28/03/24.
//

#pragma once
#include <string>
#include <vector>
#include <ntgcalls/signaling/messages/message.hpp>

namespace ntgcalls::signaling::messages {

    class InitialSetupMessage final: public Message {
    public:
        struct DtlsFingerprint {
            std::string hash;
            std::string setup;
            std::string fingerprint;
        };

        std::string ufrag;
        std::string pwd;
        bool supports_renomination = false;
        std::vector<DtlsFingerprint> fingerprints;

        [[nodiscard]] bytes::binary serialize() const override;

        static std::unique_ptr<InitialSetupMessage> deserialize(const bytes::binary& data);
    };

} // ntgcalls::signaling::messages