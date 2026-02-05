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
        virtual ~SignalingInterface() = default;

        SignalingInterface(
            webrtc::Thread* networkThread,
            webrtc::Thread* signalingThread,
            const EncryptionKey &key,
            DataEmitter onEmitData,
            DataReceiver onSignalData
        );

        void init();

        virtual void send(const bytes::binary& data) = 0;

        virtual void receive(const bytes::binary& data) = 0;

        virtual void close();

    protected:
        DataReceiver onSignalData;
        DataEmitter onEmitData;
        webrtc::Thread *networkThread, *signalingThread;

        std::vector<bytes::binary> preReadData(const bytes::binary &data, bool isRaw = false);

        bytes::binary preSendData(const bytes::binary &data, bool isRaw = false);

        virtual bool supportsCompression() const = 0;

    private:
        std::mutex mutex;
        std::unique_ptr<SignalingEncryption> signalingEncryption;

    };
} // signaling
