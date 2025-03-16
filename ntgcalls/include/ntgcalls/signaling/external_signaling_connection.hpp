//
// Created by Laky64 on 18/08/2024.
//

#pragma once
#include <rtc_base/third_party/sigslot/sigslot.h>

#include <ntgcalls/signaling/signaling_interface.hpp>

namespace signaling {
    class ExternalSignalingConnection final : public sigslot::has_slots<>, public SignalingInterface {
    public:
        ExternalSignalingConnection(
            rtc::Thread* networkThread,
            rtc::Thread* signalingThread,
            const EncryptionKey &key,
            const DataEmitter& onEmitData,
            const DataReceiver& onSignalData
        );

        void send(const bytes::binary& data) override;

        void receive(const bytes::binary& data) const override;

    protected:
        [[nodiscard]] bool supportsCompression() const override;
    };
} // signaling
