//
// Created by Laky64 on 16/03/2024.
//

#include <ntgcalls/signaling/signaling_sctp_connection.hpp>

#include <api/environment/environment_factory.h>

namespace signaling {
    SignalingSctpConnection::SignalingSctpConnection(
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread,
        const webrtc::Environment& env,
        const EncryptionKey &key,
        const DataEmitter& onEmitData,
        const DataReceiver& onSignalData,
        const bool allowCompression
    ): SignalingInterface(networkThread, signalingThread, key, onEmitData, onSignalData), allowCompression(allowCompression) {
        networkThread->BlockingCall([&] {
            packetTransport = std::make_unique<SignalingPacketTransport>(onEmitData);
            sctpTransportFactory = std::make_unique<cricket::SctpTransportFactory>(networkThread);
            sctpTransport = sctpTransportFactory->CreateSctpTransport(env, packetTransport.get());
            sctpTransport->OpenStream(0, webrtc::PriorityValue(webrtc::Priority::kVeryLow));
            sctpTransport->SetDataChannelSink(this);
            sctpTransport->Start(5000, 5000, 262144);
        });
    }

    SignalingSctpConnection::~SignalingSctpConnection() {
        networkThread->BlockingCall([&] {
            sctpTransport = nullptr;
            sctpTransportFactory = nullptr;
            packetTransport = nullptr;
        });
    }

    void SignalingSctpConnection::receive(const bytes::binary& data) const {
        networkThread->BlockingCall([&] {
            packetTransport->receiveData(data);
        });
    }

    void SignalingSctpConnection::send(const bytes::binary& data) {
        networkThread->BlockingCall([&] {
            const auto encryptedData = preSendData(data);
            if (isReadyToSend) {
                webrtc::SendDataParams params;
                params.type = webrtc::DataMessageType::kBinary;
                params.ordered = true;

                rtc::CopyOnWriteBuffer payload;
                payload.AppendData(encryptedData.data(), encryptedData.size());

                if (const auto result = sctpTransport->SendData(0, params, payload); !result.ok()) {
                    RTC_LOG(LS_ERROR) << "Failed to send data: " << result.message();
                    isReadyToSend = false;
                    pendingData.push_back(encryptedData);
                }
            } else {
                pendingData.push_back(encryptedData);
            }
        });
    }

    void SignalingSctpConnection::OnReadyToSend() {
        assert(networkThread->IsCurrent());
        isReadyToSend = true;
        for (const auto &data : pendingData) {
            webrtc::SendDataParams params;
            params.type = webrtc::DataMessageType::kBinary;
            params.ordered = true;
            rtc::CopyOnWriteBuffer payload;
            payload.AppendData(data.data(), data.size());
            if (const auto result = sctpTransport->SendData(0, params, payload); !result.ok()) {
                RTC_LOG(LS_ERROR) << "Failed to send data: " << result.message();
                pendingData.push_back(data);
                isReadyToSend = false;
            }
        }
        pendingData.clear();
    }

    void SignalingSctpConnection::OnDataReceived(int channel_id, webrtc::DataMessageType type, const rtc::CopyOnWriteBuffer& buffer) {
        assert(networkThread->IsCurrent());
        signalingThread->PostTask([this, buffer] {
            onSignalData(preReadData({buffer.data(), buffer.data() + buffer.size()}));
        });
    }

    void SignalingSctpConnection::OnTransportClosed(webrtc::RTCError error) {
        assert(networkThread->IsCurrent());
    }

    bool SignalingSctpConnection::supportsCompression() const {
        return allowCompression;
    }
} // signaling