//
// Created by Laky-64 on 18/06/26.
//

#pragma once
#include <deque>
#include <map>
#include <set>
#include <ntgcalls/e2e/chain/message_encryption.hpp>
#include <ntgcalls/models/epoch_info.hpp>
#include <ntgcalls/tl/e2e_api.hpp>
#include <rtc_base/time_utils.h>
#include <wrtc/utils/key25519.hpp>

namespace telegram::e2e {
    class SessionEncryption {
        static constexpr int32_t kMaxActiveEpochs = 15;
        static constexpr int32_t kCallPacketMagic = 0x40a6bee9;
        static constexpr int64_t kForgetEpochDelayMs = 10 * 1000;
        static constexpr int32_t kCallPacketLargeMsgIdMagic = 0x1ce56c2d;

        int64_t selfUserId;
        openssl::Key25519 privateKey;
        std::map<int32_t, uint32_t> seqno;
        std::map<int32_t, EpochInfo> epochs;
        std::map<Hash256, int32_t> epochByHash;
        std::deque<std::pair<int64_t, int32_t>> epochsToForget;
        std::map<std::pair<PublicKeyBytes, int32_t>, std::set<uint32_t>> seen;

        static void appendUInt32(bytes::binary& buffer, uint32_t value);

        static uint32_t readUInt32(const uint8_t* data);

        static void appendRaw(bytes::binary& buffer, bytes::const_span value);

        static bool validChannelId(int32_t channelId);

        static bytes::binary magic(int32_t id);

        bool checkNotSeen(
            const PublicKeyBytes& publicKey,
            int32_t channelId,
            uint32_t seqnoValue
        );

        void markAsSeen(const PublicKeyBytes& publicKey, int32_t channelId, uint32_t seqnoValue);

        std::optional<bytes::binary> encryptPacketWithSecret(
            int32_t channelId,
            bytes::const_span unencryptedPart,
            bytes::const_span packet,
            bytes::const_span oneTimeSecret
        );

        std::optional<bytes::binary> decryptPacketWithSecret(
            int64_t expectedUserId,
            bytes::const_span unencryptedHeader,
            bytes::const_span unencryptedPrefix,
            bytes::const_span encryptedPacket,
            bytes::const_span oneTimeSecret,
            const chain::GroupState& groupState
        );

    public:
        SessionEncryption(int64_t selfUserId, const openssl::Key25519 &privateKey);

        std::optional<bytes::binary> encrypt(int32_t channelId, bytes::const_span data, size_t unencryptedPrefixLength);

        std::optional<bytes::binary> decrypt(int64_t userId, bytes::const_span packet);

        void forgetSharedKey(int32_t epoch, const Hash256&);

        bool addSharedKey(int32_t epoch, const Hash256& epochHash, bytes::binary secret, chain::GroupState groupState);

        void sync();

        [[nodiscard]] int64_t userId() const;
    };
} // telegram::e2e
