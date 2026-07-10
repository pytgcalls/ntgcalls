//
// Created by Lauren on 19/06/26.
//

#pragma once
#include <map>
#include <ntgcalls/e2e/chain/blockchain.hpp>
#include <ntgcalls/tl/e2e_api.hpp>
#include <ntgcalls/tl/tl.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/key25519.hpp>

namespace ntgcalls::e2e {
    class SessionVerification {
        enum class Phase {
            Commit,
            Reveal,
            End
        };

        int64_t self_user_id_;
        int32_t height_ = -1;
        tl::Hash256 self_nonce_{};
        bool sent_reveal_ = false;
        tl::Hash256 last_block_hash_{};
        Phase phase_ = Phase::Commit;
        openssl::Key25519 private_key_;
        std::map<int64_t, tl::Hash256> revealed_;
        std::map<int64_t, tl::Hash256> committed_;
        std::vector<bytes::binary> pending_outbound_;
        std::optional<bytes::binary> emoji_hash_value_;
        std::map<int64_t, tl::PublicKeyBytes> participant_keys_;
        std::map<int32_t, std::vector<bytes::binary>> delayed_;

        static tl::Hash256 sha256(bytes::const_span data);

        static tl::Hash256 random_nonce();

        static bytes::binary data_to_sign(const chain::GroupBroadcastNonceCommit& commit);

        static bytes::binary data_to_sign(const chain::GroupBroadcastNonceReveal& reveal);

        bool process_commit(const chain::GroupBroadcastNonceCommit& commit);

        bool process_reveal(const chain::GroupBroadcastNonceReveal& reveal);

        bool process_broadcast(const chain::GroupBroadcast& broadcast);

        void emit_reveal_if_needed();

    public:
        SessionVerification(int64_t self_user_id, const openssl::Key25519 &private_key);

        bool receive_inbound_message(bytes::const_span message);

        [[nodiscard]] std::optional<bytes::binary> emoji_hash() const;

        std::vector<bytes::binary> pull_outbound_messages();

        void on_new_main_block(const chain::Blockchain& blockchain);
    };
} // ntgcalls::e2e
