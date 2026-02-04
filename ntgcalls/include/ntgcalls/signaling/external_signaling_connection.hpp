//
// Created by Laky64 on 18/08/2024.
//

#pragma once

#include <ntgcalls/signaling/signaling_interface.hpp>

namespace signaling {
    class ExternalSignalingConnection final : public SignalingInterface {
    public:
        ExternalSignalingConnection(
            webrtc::Thread* networkThread,
            webrtc::Thread* signalingThread,
            const EncryptionKey &key,
            const DataEmitter& onEmitData,
            const DataReceiver& onSignalData
        );

        void send(const bytes::binary& data) override;

        void receive(const bytes::binary& data) override;

    protected:
        [[nodiscard]] bool supportsCompression() const override;
    };
} // signaling
