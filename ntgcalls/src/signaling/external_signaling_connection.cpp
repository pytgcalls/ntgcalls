//
// Created by Laky64 on 18/08/2024.
//

#include <ntgcalls/signaling/external_signaling_connection.hpp>

namespace signaling {
    ExternalSignalingConnection::ExternalSignalingConnection(
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread,
        const EncryptionKey &key,
        const DataEmitter& onEmitData,
        const DataReceiver& onSignalData
    ): SignalingInterface(networkThread, signalingThread, key, onEmitData, onSignalData) {}

    void ExternalSignalingConnection::send(const bytes::binary& data) {
        onEmitData(preSendData(data, true));
    }

    void ExternalSignalingConnection::receive(const bytes::binary& data) const {
        signalingThread->PostTask([this, data] {
            onSignalData(preReadData(data, true));
        });
    }

    bool ExternalSignalingConnection::supportsCompression() const {
        return false;
    }
} // signaling