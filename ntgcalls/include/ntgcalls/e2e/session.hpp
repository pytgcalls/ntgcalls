//
// Created by Lauren on 17/06/26.
//

#pragma once
#include <mutex>
#include <optional>
#include <unordered_set>
#include <ntgcalls/e2e/session_encryption.hpp>
#include <ntgcalls/e2e/session_verification.hpp>
#include <ntgcalls/e2e/subchain_request.hpp>
#include <ntgcalls/e2e/sub_chain_state.hpp>
#include <ntgcalls/e2e/chain/client_blockchain.hpp>
#include <ntgcalls/tl/e2e_api.hpp>
#include <wrtc/interfaces/media/e2e_encryptor.hpp>
#include <wrtc/utils/key25519.hpp>
#include <wrtc/utils/safe_thread.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace ntgcalls::e2e {
    class Session final: public std::enable_shared_from_this<Session>, public wrtc::interfaces::media::E2EEncryptor {
        static constexpr int kSubChainsCount = 2;
        static constexpr int kShortPollTimeoutMs = 5000;
        static constexpr int kShortPollWaitForMs = 1000;
        static constexpr auto kShortPollChainBlocksPerRequest = 50;

        std::mutex mutex_;
        bool failed_ = false;
        openssl::Key25519 private_key_;
        wrtc::utils::SafeThread& update_thread_;
        std::optional<bytes::binary> last_block_;
        std::string fingerprint_emojis_;
        int last_block_height_ = 0;
        SubChainState subchains_[kSubChainsCount];
        std::optional<chain::ClientBlockchain> blockchain_;
        std::unordered_set<int64_t> participants_set_;
        std::unique_ptr<SessionEncryption> session_encryption_;
        std::unique_ptr<SessionVerification> session_verification_;
        wrtc::utils::synchronized_callback<void(bytes::binary)> outbound_block_callback_;
        wrtc::utils::synchronized_callback<void(std::string)> update_emojis_callback_;
        wrtc::utils::synchronized_callback<void(SubchainRequest)> subchain_request_callback_;

        static bytes::array<32> random_secret();

        static std::vector<chain::Change> make_changes_for_new_state(const chain::GroupState& group_state);

        static bool is_good_magic(int32_t magic);

        static int32_t read_magic(bytes::const_span block);

        static void write_magic(bytes::binary& block, int32_t magic);

        static std::optional<bytes::binary> from_server_to_local(bytes::const_span block);

        static std::optional<bytes::binary> create_zero_block(
            const openssl::Key25519& key,
            const chain::GroupState& group_state
        );

        static std::optional<bytes::binary> create_self_add_block(
            const openssl::Key25519 &key,
            bytes::const_span previous_server_block,
            const chain::GroupParticipant &self
        );

        std::optional<bytes::binary> receive_inbound_message(bytes::const_span server_message);

        void apply(int subchain, const bytes::binary& last);

        void schedule_waiting(int subchain);

        void cancel_waiting(int subchain);

        void cancel_short_poll(int subchain);

        void check_waiting_blocks(int subchain, bool waited  = false);

        void check_for_outbound_messages();

        std::vector<bytes::binary> pull_outbound_messages();

        bool init_blockchain(bytes::const_span server_block);

        bool apply_block(bytes::const_span server_block);

        bool update_group_shared_key();

        std::optional<bytes::binary> decrypt_shared_key() const;

        void schedule_short_poll(int subchain);

        chain::GroupState current_group_state();

        std::optional<bytes::binary> emoji_hash();

        void update_emojis(const std::optional<bytes::binary>& hash);

        void refresh_from_call();

    public:
        Session(wrtc::utils::SafeThread& update_thread, int64_t user_id);

        ~Session() override;

        void set_last_block(const bytes::binary& block);

        bytes::binary make_join_block();

        void short_poll(int subchain);

        void finish_subchain_request(int subchain);

        bytes::binary public_key() const;

        void apply_blocks(
            int subchain,
            int next_offset,
            const std::vector<bytes::binary>& blocks,
            bool from_short_poll
        );

        void on_outbound_block(const std::function<void(bytes::binary)>& callback);

        void on_subchain_request(const std::function<void(SubchainRequest)>& callback);

        void on_update_emoji_hash(const std::function<void(std::string)>& callback);

        std::string get_fingerprint_emojis();

        bytes::binary encrypt(const bytes::binary& data, size_t unencrypted_prefix) override;

        bytes::binary decrypt(int64_t user_id, const bytes::binary& data) override;
    };
} // ntgcalls::e2e
