//
// Created by Laky64 on 16/03/2024.
//

#include <ntgcalls/signaling/signaling_interface.hpp>

#include <utility>

#include <ntgcalls/exceptions.hpp>

namespace signaling {
    void SignalingInterface::close() {
        signalingEncryption->onServiceMessage(nullptr);
        onEmitData = nullptr;
        onSignalData = nullptr;
        signalingThread->BlockingCall([&] {});
        signalingEncryption = nullptr;
    }

    SignalingInterface::SignalingInterface(
        webrtc::Thread* networkThread,
        webrtc::Thread* signalingThread,
        const EncryptionKey& key,
        DataEmitter onEmitData,
        DataReceiver onSignalData
    ): onSignalData(std::move(onSignalData)), onEmitData(std::move(onEmitData)), networkThread(networkThread), signalingThread(signalingThread) {
        signalingEncryption = std::make_unique<SignalingEncryption>(key);
    }

    void SignalingInterface::init() {
        std::weak_ptr weak(shared_from_this());
        signalingEncryption->onServiceMessage([weak](const int delayMs, int cause) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            if (delayMs == 0) {
                strong->signalingThread->PostTask([weak, cause] {
                    const auto strongThread = weak.lock();
                    if (!strongThread) {
                        return;
                    }
                    std::lock_guard lock(strongThread->mutex);
                    if (const auto service = strongThread->signalingEncryption->prepareForSendingService(cause)) {
                        strongThread->onEmitData(*service);
                    }
                });
            } else {
                strong->signalingThread->PostDelayedTask([weak, cause] {
                    const auto strongThread = weak.lock();
                    if (!strongThread) {
                        return;
                    }
                    std::lock_guard lock(strongThread->mutex);
                    if (const auto service = strongThread->signalingEncryption->prepareForSendingService(cause)) {
                        strongThread->onEmitData(*service);
                    }
                }, webrtc::TimeDelta::Millis(delayMs));
            }
        });
    }

    std::vector<bytes::binary> SignalingInterface::preReadData(const bytes::binary &data, const bool isRaw) {
        std::lock_guard lock(mutex);
        RTC_LOG(LS_VERBOSE) << "Decrypting packets";
        const auto raw = signalingEncryption->decrypt(webrtc::CopyOnWriteBuffer(data.data(), data.size()), isRaw);
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

    bytes::binary SignalingInterface::preSendData(const bytes::binary &data, const bool isRaw) {
        std::lock_guard lock(mutex);
        auto packetData = data;
        if (supportsCompression()) {
            RTC_LOG(LS_VERBOSE) << "Compressing packet";
            packetData = std::move(bytes::GZip::zip(packetData));
        }
        RTC_LOG(LS_VERBOSE) << "Encrypting packet";
        const auto packet = signalingEncryption->encrypt(webrtc::CopyOnWriteBuffer(packetData.data(), packetData.size()), isRaw);
        if (!packet.has_value()) {
            RTC_LOG(LS_ERROR) << "Failed to encrypt packet";
            return {};
        }
        RTC_LOG(LS_VERBOSE) << "Packet encrypted";
        return *packet;
    }
} // signaling