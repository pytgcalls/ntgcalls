//
// Created by Laky-64 on 19/06/26.
//

#include <algorithm>
#include <ranges>
#include <ntgcalls/e2e/session_verification.hpp>
#include <wrtc/utils/encryption.hpp>
#include <wrtc/utils/random.hpp>

namespace telegram::e2e {
    SessionVerification::SessionVerification(const int64_t selfUserId, const openssl::Key25519 &privateKey):
        selfUserId(selfUserId), privateKey(privateKey) {}

    Hash256 SessionVerification::sha256(const bytes::const_span data) {
        Hash256 result{};
        const auto digest = openssl::Sha256::Digest(data);
        std::copy_n(reinterpret_cast<const uint8_t*>(digest.data()), result.size(), result.begin());
        return result;
    }

    Hash256 SessionVerification::randomNonce() {
        Hash256 nonce{};
        bytes::RandomFill(bytes::span(reinterpret_cast<std::byte*>(nonce.data()), nonce.size()));
        return nonce;
    }

    bytes::binary SessionVerification::dataToSign(const chain::GroupBroadcastNonceCommit &commit) {
        chain::GroupBroadcastNonceCommit copy = commit;
        copy.signature = {};
        TlWriter w;
        copy.storeBoxed(w);
        return w.result();
    }

    bytes::binary SessionVerification::dataToSign(const chain::GroupBroadcastNonceReveal &reveal) {
        chain::GroupBroadcastNonceReveal copy = reveal;
        copy.signature = {};
        TlWriter w;
        copy.storeBoxed(w);
        return w.result();
    }

    bool SessionVerification::processCommit(const chain::GroupBroadcastNonceCommit &commit) {
        if (phase != Phase::Commit) {
            return false;
        }
        const auto it = participantKeys.find(commit.user_id);
        if (it == participantKeys.end()) {
            return false;
        }
        if (!openssl::Key25519::Verify(
            bytes::view(it->second),
            bytes::view(dataToSign(commit)),
            bytes::view(commit.signature))
        ) {
            return false;
        }
        if (committed.contains(commit.user_id)) {
            return false;
        }
        committed[commit.user_id] = commit.nonce_hash;
        if (committed.size() == participantKeys.size()) {
            phase = Phase::Reveal;
        }
        return true;
    }

    bool SessionVerification::processReveal(const chain::GroupBroadcastNonceReveal& reveal) {
        if (phase != Phase::Reveal) {
            return false;
        }
        const auto it = participantKeys.find(reveal.user_id);
        if (it == participantKeys.end()) {
            return false;
        }
        if (!openssl::Key25519::Verify(
            bytes::view(it->second),
            bytes::view(dataToSign(reveal)),
            bytes::view(reveal.signature))
        ) {
            return false;
        }
        if (revealed.contains(reveal.user_id)) {
            return false;
        }
        if (
            const auto committedIt = committed.find(reveal.user_id);
            committedIt == committed.end() || sha256(bytes::view(reveal.nonce)) != committedIt->second
         ) {
            return false;
        }
        revealed[reveal.user_id] = reveal.nonce;
        if (revealed.size() == participantKeys.size()) {
            std::vector<Hash256> nonces;
            nonces.reserve(revealed.size());
            for (const auto &nonce: revealed | std::views::values) {
                nonces.push_back(nonce);
            }
            std::ranges::sort(nonces);
            bytes::binary fullNonce;
            fullNonce.reserve(nonces.size() * 32);
            for (const auto& nonce : nonces) {
                fullNonce.insert(fullNonce.end(), nonce.begin(), nonce.end());
            }
            const auto digest = openssl::Hmac::Sha512(bytes::view(fullNonce), bytes::view(lastBlockHash));
            emojiHashValue = bytes::binary(digest.begin(), digest.end());
            phase = Phase::End;
        }
        return true;
    }

    bool SessionVerification::processBroadcast(const chain::GroupBroadcast &broadcast) {
        Hash256 broadcastHash{};
        std::visit(
            [&broadcastHash](const auto& value) {
                broadcastHash = value.chain_hash;
            },
            broadcast.value
        );
        if (broadcastHash != lastBlockHash) {
            return false;
        }
        return std::visit([this]<typename T>(const T& value) {
            if constexpr (std::is_same_v<T, chain::GroupBroadcastNonceCommit>) {
                return processCommit(value);
            } else {
                return processReveal(value);
            }
        }, broadcast.value);
    }

    void SessionVerification::emitRevealIfNeeded() {
        if (phase == Phase::Reveal && !sentReveal) {
            sentReveal = true;
            chain::GroupBroadcastNonceReveal reveal;
            reveal.user_id = selfUserId;
            reveal.chain_height = height;
            reveal.chain_hash = lastBlockHash;
            reveal.nonce = selfNonce;
            reveal.signature = privateKey.sign(bytes::view(dataToSign(reveal)));

            TlWriter writer;
            reveal.storeBoxed(writer);
            pendingOutbound.push_back(writer.result());
        }
    }

    bool SessionVerification::receiveInboundMessage(const bytes::const_span message) {
        TlReader reader(message);
        auto broadcast = chain::GroupBroadcast::fetchBoxed(reader);
        if (!reader.finish()) {
            return false;
        }
        int32_t chainHeight = -1;
        std::visit([&chainHeight](const auto& value) {
            chainHeight = value.chain_height;
        }, broadcast.value);

        if (chainHeight < height) {
            return true;
        }
        if (chainHeight > height) {
            const auto data = reinterpret_cast<const uint8_t*>(message.data());
            delayed[chainHeight].emplace_back(data, data + message.size());
            return true;
        }
        const auto applied = processBroadcast(broadcast);
        emitRevealIfNeeded();
        return applied;
    }

    std::optional<bytes::binary> SessionVerification::emojiHash() const {
        return emojiHashValue;
    }

    std::vector<bytes::binary> SessionVerification::pullOutboundMessages() {
        std::vector<bytes::binary> result;
        std::swap(result, pendingOutbound);
        return result;
    }

    void SessionVerification::onNewMainBlock(const chain::Blockchain &blockchain) {
        selfNonce = randomNonce();
        height = blockchain.height();
        lastBlockHash = blockchain.hash();
        phase = Phase::Commit;
        committed.clear();
        revealed.clear();
        emojiHashValue.reset();
        sentReveal = false;

        participantKeys.clear();
        for (const auto& participant : blockchain.currentGroupState().participants) {
            participantKeys.emplace(participant.user_id, participant.public_key);
        }

        chain::GroupBroadcastNonceCommit commit;
        commit.user_id = selfUserId;
        commit.chain_height = height;
        commit.chain_hash = lastBlockHash;
        commit.nonce_hash = sha256(bytes::view(selfNonce));
        commit.signature = privateKey.sign(bytes::view(dataToSign(commit)));

        TlWriter writer;
        commit.storeBoxed(writer);
        pendingOutbound.clear();
        pendingOutbound.push_back(writer.result());

        if (const auto it = delayed.find(height); it != delayed.end()) {
            const auto pending = std::move(it->second);
            delayed.erase(it);
            for (const auto& message : pending) {
                TlReader reader(bytes::view(message));
                const auto broadcast = chain::GroupBroadcast::fetchBoxed(reader);
                if (!reader.finish()) {
                    processBroadcast(broadcast);
                }
            }
            emitRevealIfNeeded();
        }
    }
} // telegram::e2e