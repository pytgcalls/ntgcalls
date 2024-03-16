//
// Created by Laky64 on 16/03/2024.
//

#include "signaling_interface.hpp"

#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {
    SignalingInterface::SignalingInterface(
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread,
        const bool isOutGoing,
        const Key& key,
        const std::function<void(const bytes::binary&)>& onEmitData,
        const std::function<void(const std::optional<bytes::binary>&)>& onSignalData
    ): onSignalData(onSignalData), onEmitData(onEmitData), networkThread(networkThread), signalingThread(signalingThread) {
        signalingEncryption = std::make_shared<SignalingEncryption>(isOutGoing, key);
    }

    SignalingInterface::ProtocolVersion SignalingInterface::signalingVersion(const std::vector<std::string>& versions) {
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

    std::optional<bytes::binary> SignalingInterface::preProcessData(const bytes::binary& data, bool isOut) const {
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
            if (const auto unzipped = bytes::GZip::unzip(decryptedData, 2 * 1024 * 1024); unzipped.has_value()) {
                decryptedData = unzipped.value();
            } else {
                return std::nullopt;
            }
        }
        return decryptedData;
    }
} // ntgcalls