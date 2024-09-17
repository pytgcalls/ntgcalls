//
// Created by Laky64 on 16/03/2024.
//

#include <ntgcalls/signaling/signaling_interface.hpp>

#include <utility>

#include <ntgcalls/exceptions.hpp>

namespace signaling {
    SignalingInterface::~SignalingInterface() {
        signalingEncryption = nullptr;
    }

    SignalingInterface::SignalingInterface(
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread,
        const EncryptionKey& key,
        DataEmitter onEmitData,
        DataReceiver onSignalData
    ): onSignalData(std::move(onSignalData)), onEmitData(std::move(onEmitData)), networkThread(networkThread), signalingThread(signalingThread) {
        signalingEncryption = std::make_shared<SignalingEncryption>(key);
        signalingEncryptionWeak = signalingEncryption;
        signalingEncryption->onServiceMessage([this](const int delayMs, int cause) {
            if (delayMs == 0) {
                this->signalingThread->PostTask([this, cause] {
                    const auto strong = signalingEncryptionWeak.lock();
                    if (!strong) {
                        return;
                    }
                    if (const auto service = strong->prepareForSendingService(cause)) {
                        this->onEmitData(*service);
                    }
                });
            } else {
                this->signalingThread->PostDelayedTask([this, cause] {
                    const auto strong = signalingEncryptionWeak.lock();
                    if (!strong) {
                        return;
                    }
                    if (const auto service = strong->prepareForSendingService(cause)) {
                        this->onEmitData(*service);
                    }
                }, webrtc::TimeDelta::Millis(delayMs));
            }
        });
    }

    std::vector<bytes::binary> SignalingInterface::preReadData(const bytes::binary &data, const bool isRaw) const {
        RTC_LOG(LS_VERBOSE) << "Decrypting packets";
        const auto raw = signalingEncryption->decrypt(rtc::CopyOnWriteBuffer(data.data(), data.size()), isRaw);
        if (raw.empty()) {
            return {};
        }
        RTC_LOG(LS_VERBOSE) << "Packets decrypted";
        std::vector<bytes::binary> packets;
        for (auto& packet : raw) {
            auto decryptedData = bytes::binary(packet.data(), packet.data() + packet.size());
            if (bytes::GZip::isGzip(decryptedData)) {
                RTC_LOG(LS_VERBOSE) << "Decompressing packet";
                if (auto unzipped = bytes::GZip::unzip(decryptedData, 2 * 1024 * 1024); unzipped.has_value()) {
                    packets.push_back(unzipped.value());
                    continue;
                }
                RTC_LOG(LS_ERROR) << "Failed to decompress packet";
                continue;
            }
            packets.push_back(decryptedData);
        }
        return packets;
    }

    bytes::binary SignalingInterface::preSendData(const bytes::binary &data, const bool isRaw) const {
        auto packetData = data;
        if (supportsCompression()) {
            RTC_LOG(LS_VERBOSE) << "Compressing packet";
            packetData = std::move(bytes::GZip::zip(packetData));
        }
        RTC_LOG(LS_VERBOSE) << "Encrypting packet";
        const auto packet = signalingEncryption->encrypt(rtc::CopyOnWriteBuffer(packetData.data(), packetData.size()), isRaw);
        if (!packet.has_value()) {
            RTC_LOG(LS_ERROR) << "Failed to encrypt packet";
            return {};
        }
        RTC_LOG(LS_VERBOSE) << "Packet encrypted";
        return *packet;
    }
} // signaling