//
// Created by Laky64 on 16/03/2024.
//

#pragma once
#include "signaling_interface.hpp"

namespace ntgcalls {

    class SignalingV1 final: public SignalingInterface {
    public:
        SignalingV1(
            rtc::Thread* networkThread,
            rtc::Thread* signalingThread,
            const EncryptionKey &key,
            const std::function<void(const bytes::binary&)>& onEmitData,
            const std::function<void(const std::optional<bytes::binary>&)>& onSignalData
        ): SignalingInterface(networkThread, signalingThread, key, onEmitData, onSignalData) {}

        void send(const bytes::binary& data) override;

        void receive(const bytes::binary& data) const override;

    protected:
        [[nodiscard]] bool supportsCompression() const override;
    };

} // ntgcalls
