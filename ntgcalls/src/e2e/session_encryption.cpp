//
// Created by Laky-64 on 18/06/26.
//

#include <ranges>
#include <ntgcalls/e2e/session_encryption.hpp>
#include <ntgcalls/e2e/state.hpp>
#include <ntgcalls/e2e/chain/message_encryption.hpp>
#include <wrtc/utils/random.hpp>

namespace telegram::e2e {
    SessionEncryption::SessionEncryption(
        const int64_t selfUserId,
        const openssl::Key25519 &privateKey
    ) : selfUserId(selfUserId), privateKey(privateKey) {}

    void SessionEncryption::appendUInt32(bytes::binary &buffer, const uint32_t value) {
        for (int i = 0; i < 4; ++i) {
            buffer.push_back(static_cast<uint8_t>(value >> i * 8 & 0xff));
        }
    }

    uint32_t SessionEncryption::readUInt32(const uint8_t *data) {
        return static_cast<uint32_t>(data[0]) |
               static_cast<uint32_t>(data[1]) << 8 |
               static_cast<uint32_t>(data[2]) << 16 |
               static_cast<uint32_t>(data[3]) << 24;
    }

    void SessionEncryption::appendRaw(bytes::binary &buffer, const bytes::const_span value) {
        const auto data = reinterpret_cast<const uint8_t*>(value.data());
        buffer.insert(buffer.end(), data, data + value.size());
    }

    bool SessionEncryption::validChannelId(const int32_t channelId) {
        return channelId >= 0 && channelId <= 1023;
    }

    bytes::binary SessionEncryption::magic(const int32_t id) {
        bytes::binary result;
        appendUInt32(result, static_cast<uint32_t>(id));
        return result;
    }

    bool SessionEncryption::checkNotSeen(const PublicKeyBytes& publicKey, const int32_t channelId,
        const uint32_t seqnoValue) {
        const auto& s = seen[{publicKey, channelId}];
        if (s.empty()) {
            return true;
        }
        if (seqnoValue < *s.begin()) {
            return false;
        }
        return !s.contains(seqnoValue);
    }

    void SessionEncryption::markAsSeen(const PublicKeyBytes& publicKey, const int32_t channelId,
        const uint32_t seqnoValue) {
        auto& s = seen[{publicKey, channelId}];
        s.insert(seqnoValue);
        while (s.size() > 1024 || (!s.empty() && *s.begin() + 1024 < seqnoValue)) {
            s.erase(s.begin());
        }
    }

    void SessionEncryption::sync() {
        const auto now = webrtc::TimeMillis();
        while (!epochsToForget.empty() && (epochsToForget.front().first <= now || epochs.size() > kMaxActiveEpochs)) {
            const auto epoch = epochsToForget.front().second;
            if (const auto it = epochs.find(epoch); it != epochs.end()) {
                epochByHash.erase(it->second.epochHash);
                epochs.erase(it);
            }
            epochsToForget.pop_front();
        }
    }

    int64_t SessionEncryption::userId() const {
        return selfUserId;
    }

    std::optional<bytes::binary> SessionEncryption::encryptPacketWithSecret(
        const int32_t channelId,
        const bytes::const_span unencryptedPart,
        const bytes::const_span packet,
        const bytes::const_span oneTimeSecret
    ) {
        if (!validChannelId(channelId)) {
            return std::nullopt;
        }
        auto& channelSeqno = seqno[channelId];
        if (channelSeqno == std::numeric_limits<uint32_t>::max()) {
            return std::nullopt;
        }
        ++channelSeqno;

        bytes::binary payload;
        appendUInt32(payload, static_cast<uint32_t>(channelId));
        appendUInt32(payload, channelSeqno);
        appendRaw(payload, packet);

        bytes::binary extra = magic(kCallPacketMagic);
        appendRaw(extra, unencryptedPart);

        std::array<uint8_t, 32> largeMsgId{};
        const auto encrypted = chain::MessageEncryption::encryptData(bytes::view(payload), oneTimeSecret, bytes::view(extra), &largeMsgId);

        bytes::binary toSign = magic(kCallPacketLargeMsgIdMagic);
        appendRaw(toSign, bytes::view(largeMsgId));
        const auto signature = privateKey.sign(bytes::view(toSign));

        bytes::binary result = encrypted;
        appendRaw(result, bytes::view(signature));
        return result;
    }

    std::optional<bytes::binary> SessionEncryption::decryptPacketWithSecret(
        const int64_t expectedUserId,
        const bytes::const_span unencryptedHeader,
        const bytes::const_span unencryptedPrefix,
        const bytes::const_span encryptedPacket,
        const bytes::const_span oneTimeSecret,
        const chain::GroupState &groupState
    ) {
        const auto participant = State::findParticipant(groupState, expectedUserId);
        if (!participant) {
            return std::nullopt;
        }
        if (encryptedPacket.size() < 64) {
            return std::nullopt;
        }
        const auto signature = encryptedPacket.subspan(encryptedPacket.size() - 64, 64);
        const auto payloadCipher = encryptedPacket.subspan(0, encryptedPacket.size() - 64);

        bytes::binary extra = magic(kCallPacketMagic);
        appendRaw(extra, unencryptedHeader);
        appendRaw(extra, unencryptedPrefix);

        std::array<uint8_t, 32> largeMsgId{};
        const auto payload = chain::MessageEncryption::decryptData(
            payloadCipher,
            oneTimeSecret,
            bytes::view(extra),
            &largeMsgId
        );
        if (!payload) {
            return std::nullopt;
        }

        bytes::binary toVerify = magic(kCallPacketLargeMsgIdMagic);
        appendRaw(toVerify, bytes::view(largeMsgId));
        if (!openssl::Key25519::Verify(bytes::view(participant->public_key), bytes::view(toVerify), signature)) {
            return std::nullopt;
        }

        if (payload->size() < 8) {
            return std::nullopt;
        }
        const auto channelId = static_cast<int32_t>(readUInt32(payload->data()));
        const auto seqnoValue = readUInt32(payload->data() + 4);
        if (!validChannelId(channelId)) {
            return std::nullopt;
        }
        if (!checkNotSeen(participant->public_key, channelId, seqnoValue)) {
            return std::nullopt;
        }
        markAsSeen(participant->public_key, channelId, seqnoValue);

        bytes::binary result(unencryptedPrefix.size() + payload->size() - 8);
        std::copy_n(reinterpret_cast<const uint8_t*>(unencryptedPrefix.data()), unencryptedPrefix.size(), result.begin());
        std::copy(payload->begin() + 8, payload->end(), result.begin() + static_cast<std::ptrdiff_t>(unencryptedPrefix.size()));
        return result;
    }

    std::optional<bytes::binary> SessionEncryption::encrypt(const int32_t channelId, const bytes::const_span data, const size_t unencryptedPrefixLength) {
        sync();
        if (unencryptedPrefixLength > data.size() || unencryptedPrefixLength >= 1 << 16) {
            return std::nullopt;
        }
        if (epochs.empty()) {
            return std::nullopt;
        }
        const auto prefix = data.subspan(0, unencryptedPrefixLength);
        const auto decrypted = data.subspan(unencryptedPrefixLength);

        bytes::binary headerA;
        appendUInt32(headerA, static_cast<uint32_t>(epochs.size()));
        for (const auto &info: epochs | std::views::values) {
            headerA.insert(headerA.end(), info.epochHash.begin(), info.epochHash.end());
        }

        std::array<uint8_t, 32> oneTimeSecret{};
        bytes::RandomFill(bytes::span(reinterpret_cast<std::byte*>(oneTimeSecret.data()), oneTimeSecret.size()));

        bytes::binary unencryptedPart = headerA;
        appendRaw(unencryptedPart, prefix);
        const auto encryptedPacket = encryptPacketWithSecret(channelId, bytes::view(unencryptedPart), decrypted, bytes::view(oneTimeSecret));
        if (!encryptedPacket) {
            return std::nullopt;
        }

        bytes::binary headerB;
        for (const auto &info: epochs | std::views::values) {
            const auto encryptedHeader = chain::MessageEncryption::encryptHeader(
                bytes::view(oneTimeSecret),
                bytes::view(*encryptedPacket),
                bytes::view(info.secret)
            );
            if (!encryptedHeader || encryptedHeader->size() != 32) {
                return std::nullopt;
            }
            appendRaw(headerB, bytes::view(*encryptedHeader));
        }

        bytes::binary result;
        appendRaw(result, prefix);
        appendRaw(result, bytes::view(headerA));
        appendRaw(result, bytes::view(headerB));
        appendRaw(result, bytes::view(*encryptedPacket));
        appendUInt32(result, static_cast<uint32_t>(unencryptedPrefixLength));
        return result;
    }

    std::optional<bytes::binary> SessionEncryption::decrypt(const int64_t userId, const bytes::const_span packet) {
        sync();
        if (packet.size() < 4) {
            return std::nullopt;
        }
        const auto base = reinterpret_cast<const uint8_t*>(packet.data());
        const auto unencryptedPrefixSize = readUInt32(base + packet.size() - 4);
        const auto body = packet.subspan(0, packet.size() - 4);
        if (unencryptedPrefixSize > body.size() || unencryptedPrefixSize >= 1 << 16) {
            return std::nullopt;
        }
        const auto unencryptedPrefix = body.subspan(0, unencryptedPrefixSize);
        const auto encryptedData = body.subspan(unencryptedPrefixSize);
        if (userId == selfUserId) {
            return std::nullopt;
        }
        if (encryptedData.size() < 4) {
            return std::nullopt;
        }
        const auto encBase = reinterpret_cast<const uint8_t*>(encryptedData.data());
        const auto head = readUInt32(encBase);
        const auto epochsN = head & 0xff;
        const auto version = head >> 8 & 0xff;
        if (const auto reserved = head >> 16; version != 0 || reserved != 0 || epochsN > static_cast<uint32_t>(kMaxActiveEpochs)) {
            return std::nullopt;
        }
        const size_t headerSize = 4 + static_cast<size_t>(epochsN) * 32;
        const size_t bodySize = headerSize + static_cast<size_t>(epochsN) * 32;
        if (encryptedData.size() < bodySize) {
            return std::nullopt;
        }
        const auto unencryptedHeader = encryptedData.subspan(0, headerSize);
        const auto encryptedPacket = encryptedData.subspan(bodySize);

        for (uint32_t i = 0; i < epochsN; ++i) {
            Hash256 epochHash{};
            std::copy_n(encBase + 4 + static_cast<size_t>(i) * 32, 32, epochHash.begin());
            const auto headerView = encryptedData.subspan(headerSize + static_cast<size_t>(i) * 32, 32);

            const auto byHash = epochByHash.find(epochHash);
            if (byHash == epochByHash.end()) {
                continue;
            }
            const auto epochIt = epochs.find(byHash->second);
            if (epochIt == epochs.end()) {
                continue;
            }
            const auto& info = epochIt->second;
            const auto oneTimeSecret = chain::MessageEncryption::decryptHeader(headerView, encryptedPacket, bytes::view(info.secret));
            if (!oneTimeSecret) {
                continue;
            }
            return decryptPacketWithSecret(
                userId,
                unencryptedHeader,
                unencryptedPrefix,
                encryptedPacket,
                bytes::view(*oneTimeSecret),
                info.groupState
            );
        }
        return std::nullopt;
    }

    void SessionEncryption::forgetSharedKey(const int32_t epoch, const Hash256 &) {
        sync();
        epochsToForget.emplace_back(webrtc::TimeMillis() + kForgetEpochDelayMs, epoch);
    }

    bool SessionEncryption::addSharedKey(const int32_t epoch, const Hash256 &epochHash, bytes::binary secret,
        chain::GroupState groupState) {
        sync();
        const auto self = State::findParticipant(groupState, privateKey.publicKeyBytes());
        if (!self || self->user_id != selfUserId) {
            return false;
        }
        epochByHash[epochHash] = epoch;
        epochs.insert_or_assign(epoch, EpochInfo{epoch, epochHash, self->user_id, std::move(secret), std::move(groupState)});
        return true;
    }
} // telegram::e2e