//
// Created by Lauren on 18/06/26.
//

#pragma once
#include <deque>
#include <map>
#include <set>
#include <ntgcalls/e2e/chain/message_encryption.hpp>
#include <ntgcalls/e2e/epoch_info.hpp>
#include <ntgcalls/tl/e2e_api.hpp>
#include <rtc_base/time_utils.h>
#include <wrtc/utils/key25519.hpp>

namespace ntgcalls::e2e {
    class SessionEncryption {
        static constexpr int32_t kMaxActiveEpochs = 15;
        static constexpr int32_t kCallPacketMagic = 0x40a6bee9;
        static constexpr int64_t kForgetEpochDelayMs = 10 * 1000;
        static constexpr int32_t kCallPacketLargeMsgIdMagic = 0x1ce56c2d;

        int64_t self_user_id_;
        openssl::Key25519 private_key_;
        std::map<int32_t, uint32_t> seqno_;
        std::map<int32_t, EpochInfo> epochs_;
        std::map<tl::Hash256, int32_t> epoch_by_hash_;
        std::deque<std::pair<int64_t, int32_t>> epochs_to_forget_;
        std::map<std::pair<tl::PublicKeyBytes, int32_t>, std::set<uint32_t>> seen_;

        static void append_uint32(bytes::binary& buffer, uint32_t value);

        static uint32_t read_uint32(const uint8_t* data);

        static void append_raw(bytes::binary& buffer, bytes::const_span value);

        static bool valid_channel_id(int32_t channel_id);

        static bytes::binary magic(int32_t id);

        bool check_not_seen(
            const tl::PublicKeyBytes& public_key,
            int32_t channel_id,
            uint32_t seqno_value
        );

        void mark_as_seen(const tl::PublicKeyBytes& public_key, int32_t channel_id, uint32_t seqno_value);

        std::optional<bytes::binary> encrypt_packet_with_secret(
            int32_t channel_id,
            bytes::const_span unencrypted_part,
            bytes::const_span packet,
            bytes::const_span one_time_secret
        );

        std::optional<bytes::binary> decrypt_packet_with_secret(
            int64_t expected_user_id,
            bytes::const_span unencrypted_header,
            bytes::const_span unencrypted_prefix,
            bytes::const_span encrypted_packet,
            bytes::const_span one_time_secret,
            const chain::GroupState& group_state
        );

    public:
        SessionEncryption(int64_t self_user_id, const openssl::Key25519 &private_key);

        std::optional<bytes::binary> encrypt(int32_t channel_id, bytes::const_span data, size_t unencrypted_prefix_length);

        std::optional<bytes::binary> decrypt(int64_t user_id, bytes::const_span packet);

        void forget_shared_key(int32_t epoch, const tl::Hash256&);

        bool add_shared_key(int32_t epoch, const tl::Hash256& epoch_hash, bytes::binary secret, chain::GroupState group_state);

        void sync();

        [[nodiscard]] int64_t user_id() const;
    };
} // ntgcalls::e2e
