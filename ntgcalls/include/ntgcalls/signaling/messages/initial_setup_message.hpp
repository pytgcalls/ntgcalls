//
// Created by Laky64 on 28/03/2024.
//

#pragma once
#include <string>
#include <vector>

#include <ntgcalls/signaling/messages/message.hpp>

namespace signaling {

    class InitialSetupMessage final: public Message{
    public:
        struct DtlsFingerprint {
            std::string hash;
            std::string setup;
            std::string fingerprint;
        };

        std::string ufrag;
        std::string pwd;
        bool supportsRenomination = false;
        std::vector<DtlsFingerprint> fingerprints;

        [[nodiscard]] bytes::binary serialize() const override;

        static std::unique_ptr<InitialSetupMessage> deserialize(const bytes::binary& data);
    };

} // signaling