//
// Created by Laky64 on 16/03/2024.
//

#include "signaling_v1.hpp"

namespace ntgcalls {
    void SignalingV1::receive(const bytes::binary& data) const {
        signalingThread->PostTask([&] {
            onSignalData(preProcessData(data, false));
        });
    }

    bool SignalingV1::supportsCompression() const {
        return false;
    }

    void SignalingV1::send(const bytes::binary& data) {
        signalingThread->PostTask([&] {
            onEmitData(preProcessData(data, true).value());
        });
    }
} // ntgcalls