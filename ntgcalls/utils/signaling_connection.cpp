//
// Created by iraci on 14/03/2024.
//

#include "signaling_connection.hpp"

#include <iostream>
#include <memory>

namespace ntgcalls {
    SignalingConnection::SignalingConnection(
        rtc::Thread* network_thread,
        const ProtocolVersion version,
        bool isOutGoing,
        const bytes::binary& key,
        const std::function<void(const bytes::binary&)>& onEmitData,
        const std::function<void(const bytes::binary&)>& onSignalData
    ): network_thread(network_thread), onEmitData(onEmitData), onSignalData(onSignalData), version(version) {
        signaling = std::make_shared<Signaling>(isOutGoing, key);
        if (version == ProtocolVersion::V2) {
            network_thread->BlockingCall([&] {
                packetTransport = std::make_unique<SignalingPacketTransport>(onEmitData);
                sctpTransportFactory = std::make_unique<cricket::SctpTransportFactory>(network_thread);
                sctpTransport = sctpTransportFactory->CreateSctpTransport(packetTransport.get());
                sctpTransport->OpenStream(0);
                sctpTransport->SetDataChannelSink(this);
                sctpTransport->Start(5000, 5000, 262144);
            });
        }
    }

    SignalingConnection::~SignalingConnection() {
        if (version == ProtocolVersion::V2) {
            network_thread->BlockingCall([&] {
                sctpTransport.reset();
                sctpTransportFactory.reset();
                packetTransport.reset();
            });
        }
    }

    void SignalingConnection::receive(const bytes::binary &data) const {
        if (version == ProtocolVersion::V2) {
            network_thread->BlockingCall([&] {
                packetTransport->receiveData(data);
            });
        } else {
            onSignalData(signaling->decrypt(data));
        }
    }

    void SignalingConnection::send(const bytes::binary& data) {
        const auto encryptedData = signaling->encrypt(data);
        if (version == ProtocolVersion::V2) {
            network_thread->BlockingCall([&] {
                std::lock_guard lock(mutex);
                if (isReadyToSend) {
                    webrtc::SendDataParams params;
                    params.type = webrtc::DataMessageType::kBinary;
                    params.ordered = true;
                    rtc::CopyOnWriteBuffer payload;
                    payload.AppendData(encryptedData.get(), encryptedData.size());
                    if (const auto result = sctpTransport->SendData(0, params, payload); !result.ok() && result.type() == webrtc::RTCErrorType::NETWORK_ERROR) {
                        isReadyToSend = false;
                        pendingData.push_back(encryptedData);
                    }
                } else {
                    pendingData.push_back(encryptedData);
                }
            });
        } else {
            onEmitData(encryptedData);
        }
    }

    void SignalingConnection::OnReadyToSend() {
        if (version == ProtocolVersion::V2) {
            std::lock_guard lock(mutex);
            assert(_threads->getNetworkThread()->IsCurrent());
            isReadyToSend = true;
            for (const auto &data : pendingData) {
                webrtc::SendDataParams params;
                params.type = webrtc::DataMessageType::kBinary;
                params.ordered = true;
                rtc::CopyOnWriteBuffer payload;
                payload.AppendData(data.get(), data.size());
                if (const auto result = sctpTransport->SendData(0, params, payload); !result.ok()) {
                    pendingData.push_back(data);
                    isReadyToSend = false;
                }
            }
            pendingData.clear();
        }
    }

    void SignalingConnection::OnDataReceived(int channel_id, webrtc::DataMessageType type, const rtc::CopyOnWriteBuffer& buffer) {
        std::cout << "SignalingConnection::OnDataReceived" << std::endl;
        onSignalData(signaling->decrypt(bytes::binary(buffer.data(), buffer.size())));
    }
} // ntgcalls