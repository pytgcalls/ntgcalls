//
// Created by Laky64 on 16/03/2024.
//

#pragma once
#include <vector>
#include <rtc_base/thread.h>
#include <ntgcalls/signaling/crypto/signaling_encryption.hpp>

namespace signaling {
    using DataEmitter = std::function<void(const bytes::binary&)>;
    using DataReceiver = std::function<void(const std::vector<bytes::binary>&)>;

    class SignalingInterface: public std::enable_shared_from_this<SignalingInterface> {
    public:
        virtual ~SignalingInterface();

        SignalingInterface(
            rtc::Thread* networkThread,
            rtc::Thread* signalingThread,
            const EncryptionKey &key,
            DataEmitter onEmitData,
            DataReceiver onSignalData
        );

        virtual void send(const bytes::binary& data) = 0;

        virtual void receive(const bytes::binary& data) const = 0;

    protected:
        DataReceiver onSignalData;
        DataEmitter onEmitData;
        rtc::Thread *networkThread, *signalingThread;

        [[nodiscard]] std::vector<bytes::binary> preReadData(const bytes::binary &data, bool isRaw = false) const;

        [[nodiscard]] bytes::binary preSendData(const bytes::binary &data, bool isRaw = false) const;

        [[nodiscard]] virtual bool supportsCompression() const = 0;

    private:
        std::shared_ptr<SignalingEncryption> signalingEncryption;
        std::weak_ptr<SignalingEncryption> signalingEncryptionWeak;
    };
} // signaling
