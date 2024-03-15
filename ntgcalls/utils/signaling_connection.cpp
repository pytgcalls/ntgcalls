//
// Created by iraci on 14/03/2024.
//

#include "signaling_connection.hpp"

#include <memory>

namespace ntgcalls {
    SignalingConnection::SignalingConnection(
        const std::vector<std::string>& remoteVersions,
        rtc::Thread* networkThread,
        bool isOutGoing,
        const bytes::binary& key,
        const std::function<void(const bytes::binary&)>& onEmitData,
        const std::function<void(const bytes::binary&)>& onSignalData
    ): network_thread(networkThread), onEmitData(onEmitData), onSignalData(onSignalData) {
        signaling = std::make_shared<Signaling>(isOutGoing, key);
        version = signalingVersion(remoteVersions);
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
            onSignalData(preProcessData(data, false));
        }
    }

    void SignalingConnection::send(const bytes::binary& data) {
        const auto encryptedData = preProcessData(data, true);
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
        onSignalData(preProcessData(bytes::binary(buffer.data(), buffer.size()), false));
    }

    SignalingConnection::ProtocolVersion SignalingConnection::signalingVersion(const std::vector<std::string>& versions) {
        if (versions.empty()) {
            throw ConnectionError("No versions provided");
        }
        const auto it = std::ranges::find_if(versions, [](const std::string &version) {
            return version == defaultVersion;
        });
        if (const std::string foundVersion = it != versions.end() ? *it : versions[0]; foundVersion == "10.0.0") {
            return ProtocolVersion::V1;
        } else if (foundVersion == "11.0.0") {
            return ProtocolVersion::V2;
        }
        throw ConnectionError("Unsupported version");
    }

    bool SignalingConnection::supportsCompression() const {
        switch (version) {
        case ProtocolVersion::V1:
            return false;
        case ProtocolVersion::V2:
            return true;
        }
        return false;
    }

    bytes::binary SignalingConnection::preProcessData(const bytes::binary& data, const bool isOut) const {
        if (isOut) {
            auto packetData = data;
            if (supportsCompression()) {
                packetData = bytes::GZip::zip(packetData);
            }
            return signaling->encrypt(packetData);
        }
        auto decryptedData = signaling->decrypt(data);
        if (bytes::GZip::isGzip(decryptedData)) {
            decryptedData = bytes::GZip::unzip(decryptedData, 2 * 1024 * 1024);
        }
        return decryptedData;
    }
} // ntgcalls