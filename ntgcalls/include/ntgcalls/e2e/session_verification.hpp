//
// Created by Laky-64 on 19/06/26.
//

#pragma once
#include <map>
#include <ntgcalls/e2e/chain/blockchain.hpp>
#include <ntgcalls/tl/e2e_api.hpp>
#include <ntgcalls/tl/tl.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/key25519.hpp>

namespace telegram::e2e {
    class SessionVerification {
        enum class Phase {
            Commit,
            Reveal,
            End
        };

        int64_t selfUserId;
        int32_t height = -1;
        Hash256 selfNonce{};
        bool sentReveal = false;
        Hash256 lastBlockHash{};
        Phase phase = Phase::Commit;
        openssl::Key25519 privateKey;
        std::map<int64_t, Hash256> revealed;
        std::map<int64_t, Hash256> committed;
        std::vector<bytes::binary> pendingOutbound;
        std::optional<bytes::binary> emojiHashValue;
        std::map<int64_t, PublicKeyBytes> participantKeys;
        std::map<int32_t, std::vector<bytes::binary>> delayed;

        static Hash256 sha256(bytes::const_span data);

        static Hash256 randomNonce();

        static bytes::binary dataToSign(const chain::GroupBroadcastNonceCommit& commit);

        static bytes::binary dataToSign(const chain::GroupBroadcastNonceReveal& reveal);

        bool processCommit(const chain::GroupBroadcastNonceCommit& commit);

        bool processReveal(const chain::GroupBroadcastNonceReveal& reveal);

        bool processBroadcast(const chain::GroupBroadcast& broadcast);

        void emitRevealIfNeeded();

    public:
        SessionVerification(int64_t selfUserId, const openssl::Key25519 &privateKey);

        bool receiveInboundMessage(bytes::const_span message);

        [[nodiscard]] std::optional<bytes::binary> emojiHash() const;

        std::vector<bytes::binary> pullOutboundMessages();

        void onNewMainBlock(const chain::Blockchain& blockchain);
    };
} // telegram::e2e
