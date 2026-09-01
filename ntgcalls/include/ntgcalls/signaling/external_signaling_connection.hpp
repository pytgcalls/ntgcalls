//
// Created by Lauren on 18/08/24.
//

#pragma once

#include <ntgcalls/signaling/signaling_interface.hpp>

namespace ntgcalls::signaling {
    class ExternalSignalingConnection final: public SignalingInterface {
    public:
        ExternalSignalingConnection(
            wrtc::utils::SafeThread& network_thread,
            wrtc::utils::SafeThread& signaling_thread,
            const crypto::EncryptionKey& key,
            const DataEmitter& on_emit_data,
            const DataReceiver& on_signal_data
        );

        void send(const bytes::binary& data) override;

        void receive(const bytes::binary& data) override;

    protected:
        [[nodiscard]] bool supports_compression() const override;
    };
} // ntgcalls::signaling
