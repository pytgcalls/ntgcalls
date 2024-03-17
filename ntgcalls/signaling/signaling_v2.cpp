//
// Created by Laky64 on 16/03/2024.
//

#include "signaling_v2.hpp"

namespace ntgcalls {
    SignalingV2::SignalingV2(
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread,
        const EncryptionKey &key,
        const std::function<void(const bytes::binary&)>& onEmitData,
        const std::function<void(const std::optional<bytes::binary>&)>& onSignalData,
        const bool allowCompression
    ): SignalingInterface(networkThread, signalingThread, key, onEmitData, onSignalData), allowCompression(allowCompression) {
        networkThread->BlockingCall([&] {
            packetTransport = std::make_unique<SignalingPacketTransport>(onEmitData);
            sctpTransportFactory = std::make_unique<cricket::SctpTransportFactory>(networkThread);
            sctpTransport = sctpTransportFactory->CreateSctpTransport(packetTransport.get());
            sctpTransport->OpenStream(0);
            sctpTransport->SetDataChannelSink(this);
            sctpTransport->Start(5000, 5000, 262144);
        });
    }

    SignalingV2::~SignalingV2() {
        networkThread->BlockingCall([&] {
            sctpTransport.reset();
            sctpTransportFactory.reset();
            packetTransport.reset();
        });
    }

    void SignalingV2::receive(const bytes::binary& data) const {
        networkThread->BlockingCall([&] {
            packetTransport->receiveData(data);
        });
    }

    void SignalingV2::send(const bytes::binary& data) {
        networkThread->BlockingCall([&] {
            const auto encryptedData = preProcessData(data, true).value();
            if (isReadyToSend) {
                webrtc::SendDataParams params;
                params.type = webrtc::DataMessageType::kBinary;
                params.ordered = true;

                rtc::CopyOnWriteBuffer payload;
                payload.AppendData(encryptedData.data(), encryptedData.size());

                if (const auto result = sctpTransport->SendData(0, params, payload); !result.ok()) {
                    isReadyToSend = false;
                    pendingData.push_back(encryptedData);
                }
            } else {
                pendingData.push_back(encryptedData);
            }
        });
    }

    void SignalingV2::OnReadyToSend() {
        assert(networkThread->IsCurrent());
        isReadyToSend = true;
        for (const auto &data : pendingData) {
            webrtc::SendDataParams params;
            params.type = webrtc::DataMessageType::kBinary;
            params.ordered = true;
            rtc::CopyOnWriteBuffer payload;
            payload.AppendData(data.data(), data.size());
            if (const auto result = sctpTransport->SendData(0, params, payload); !result.ok()) {
                pendingData.push_back(data);
                isReadyToSend = false;
            }
        }
        pendingData.clear();
    }

    void SignalingV2::OnDataReceived(int channel_id, webrtc::DataMessageType type, const rtc::CopyOnWriteBuffer& buffer) {
        assert(networkThread->IsCurrent());
        signalingThread->PostTask([this, buffer] {
            onSignalData(preProcessData(bytes::binary(buffer.data(), buffer.data() + buffer.size()), false));
        });
    }

    void SignalingV2::OnTransportClosed(webrtc::RTCError error) {
        assert(networkThread->IsCurrent());
    }

    bool SignalingV2::supportsCompression() const {
        return allowCompression;
    }
} // ntgcalls