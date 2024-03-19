//
// Created by Laky64 on 16/03/2024.
//

#include "signaling_interface.hpp"

#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {
    SignalingInterface::SignalingInterface(
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread,
        const EncryptionKey& key,
        const std::function<void(const bytes::binary&)>& onEmitData,
        const std::function<void(const std::optional<bytes::binary>&)>& onSignalData
    ): onSignalData(onSignalData), onEmitData(onEmitData), networkThread(networkThread), signalingThread(signalingThread) {
        signalingEncryption = std::make_unique<SignalingEncryption>(key);
    }

    std::optional<bytes::binary> SignalingInterface::preProcessData(const bytes::binary &data, const bool isOut) const {
        if (isOut) {
            auto packetData = data;
            if (supportsCompression()) {
                packetData = std::move(bytes::GZip::zip(packetData));
            }
            const auto raw = signalingEncryption->encrypt(rtc::CopyOnWriteBuffer(packetData.data(), packetData.size()));
            return bytes::binary(raw.data(), raw.data() + raw.size());
        }
        const auto raw = signalingEncryption->decrypt(rtc::CopyOnWriteBuffer(data.data(), data.size()));
        if (!raw.has_value()) {
            return std::nullopt;
        }
        auto decryptedData = bytes::binary(raw->data(), raw->data() + raw->size());
        if (bytes::GZip::isGzip(decryptedData)) {
            if (auto unzipped = bytes::GZip::unzip(decryptedData, 2 * 1024 * 1024); unzipped.has_value()) {
                return unzipped.value();
            }
            return std::nullopt;
        }
        return decryptedData;
    }
} // ntgcalls