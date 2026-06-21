//
// Created by Laky-64 on 17/06/26.
//

#pragma once
#include <mutex>
#include <optional>
#include <unordered_set>
#include <ntgcalls/e2e/session_encryption.hpp>
#include <ntgcalls/e2e/session_verification.hpp>
#include <ntgcalls/e2e/chain/client_blockchain.hpp>
#include <ntgcalls/models/subchain_request.hpp>
#include <ntgcalls/models/sub_chain_state.hpp>
#include <ntgcalls/tl/e2e_api.hpp>
#include <wrtc/interfaces/media/e2e_encryptor.hpp>
#include <wrtc/utils/key25519.hpp>
#include <wrtc/utils/safe_thread.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace telegram::e2e {
    class Session final: public std::enable_shared_from_this<Session>, public wrtc::E2EEncryptor {
        static constexpr int kSubChainsCount = 2;
        static constexpr int kShortPollTimeoutMs = 5000;
        static constexpr int kShortPollWaitForMs = 1000;
        static constexpr auto kShortPollChainBlocksPerRequest = 50;

        std::mutex mutex;
        bool failed = false;
        openssl::Key25519 privateKey;
        wrtc::SafeThread& updateThread;
        std::optional<bytes::binary> lastBlock;
        int lastBlockHeight = 0;
        SubChainState subchains[kSubChainsCount];
        std::optional<chain::ClientBlockchain> blockchain;
        std::unordered_set<int64_t> participantsSet;
        std::unique_ptr<SessionEncryption> sessionEncryption;
        std::unique_ptr<SessionVerification> sessionVerification;
        wrtc::synchronized_callback<void(bytes::binary)> outboundBlockCallback;
        wrtc::synchronized_callback<void(bytes::binary)> updateEmojiHashCallback;
        wrtc::synchronized_callback<void(SubchainRequest)> subchainRequestCallback;

        static std::array<uint8_t, 32> randomSecret();

        static std::vector<chain::Change> makeChangesForNewState(const chain::GroupState& groupState);

        static bool isGoodMagic(int32_t magic);

        static int32_t readMagic(bytes::const_span block);

        static void writeMagic(bytes::binary& block, int32_t magic);

        static std::optional<bytes::binary> fromServerToLocal(bytes::const_span block);

        static std::optional<bytes::binary> createZeroBlock(
            const openssl::Key25519& key,
            const chain::GroupState& groupState
        );

        static std::optional<bytes::binary> createSelfAddBlock(
            const openssl::Key25519 &key,
            bytes::const_span previousServerBlock,
            const chain::GroupParticipant &self
        );

        std::optional<bytes::binary> receiveInboundMessage(bytes::const_span serverMessage);

        void apply(int subchain, const bytes::binary& last);

        void scheduleWaiting(int subchain);

        void cancelWaiting(int subchain);

        void cancelShortPoll(int subchain);

        void checkWaitingBlocks(int subchain, bool waited  = false);

        void checkForOutboundMessages();

        std::vector<bytes::binary> pullOutboundMessages();

        bool initBlockchain(bytes::const_span serverBlock);

        bool applyBlock(bytes::const_span serverBlock);

        bool updateGroupSharedKey();

        std::optional<bytes::binary> decryptSharedKey() const;

        void scheduleShortPoll(int subchain);

        chain::GroupState currentGroupState();

        std::optional<bytes::binary> emojiHash();

        void refreshFromCall();

    public:
        Session(wrtc::SafeThread& updateThread, int64_t userID);

        ~Session() override;

        void setLastBlock(const bytes::binary& block);

        bytes::binary makeJoinBlock();

        void shortPoll(int subchain);

        void finishSubchainRequest(int subchain);

        bytes::binary publicKey() const;

        void applyBlocks(
            int subchain,
            int nextOffset,
            const std::vector<bytes::binary>& blocks,
            bool fromShortPoll
        );

        void onOutboundBlock(const std::function<void(bytes::binary)>& callback);

        void onSubchainRequest(const std::function<void(SubchainRequest)>& callback);

        void onUpdateEmojiHash(const std::function<void(bytes::binary)>& callback);

        bytes::binary encrypt(const bytes::binary& data, size_t unencryptedPrefix) override;

        bytes::binary decrypt(int64_t userId, const bytes::binary& data) override;
    };
} // telegram::e2e
