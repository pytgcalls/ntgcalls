//
// Created by Laky64 on 28/03/2024.
//

#pragma once
#include "signaling_interface.hpp"

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
