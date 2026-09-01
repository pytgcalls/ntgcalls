//
// Created by Lauren on 19/06/26.
//

#include <algorithm>
#include <ranges>
#include <ntgcalls/e2e/session_verification.hpp>
#include <wrtc/utils/encryption.hpp>
#include <wrtc/utils/random.hpp>

namespace ntgcalls::e2e {
    SessionVerification::SessionVerification(const int64_t self_user_id, const openssl::Key25519& private_key):
    self_user_id_(self_user_id), private_key_(private_key) {}

    tl::Hash256 SessionVerification::sha256(const bytes::const_span data) {
        tl::Hash256 result{};
        const auto digest = openssl::Sha256::digest(data);
        std::copy_n(digest.data(), result.size(), result.begin());
        return result;
    }

    tl::Hash256 SessionVerification::random_nonce() {
        tl::Hash256 nonce{};
        bytes::random_fill(bytes::span(nonce.data(), nonce.size()));
        return nonce;
    }

    bytes::binary SessionVerification::data_to_sign(const chain::GroupBroadcastNonceCommit& commit) {
        chain::GroupBroadcastNonceCommit copy = commit;
        copy.signature = {};
        tl::TlWriter w;
        copy.store_boxed(w);
        return w.result();
    }

    bytes::binary SessionVerification::data_to_sign(const chain::GroupBroadcastNonceReveal& reveal) {
        chain::GroupBroadcastNonceReveal copy = reveal;
        copy.signature = {};
        tl::TlWriter w;
        copy.store_boxed(w);
        return w.result();
    }

    bool SessionVerification::process_commit(const chain::GroupBroadcastNonceCommit& commit) {
        if (phase_ != Phase::Commit) {
            return false;
        }
        const auto it = participant_keys_.find(commit.user_id);
        if (it == participant_keys_.end()) {
            return false;
        }
        if (!openssl::Key25519::verify(
                bytes::view(it->second),
                bytes::view(data_to_sign(commit)),
                bytes::view(commit.signature)
            )) {
            return false;
        }
        if (committed_.contains(commit.user_id)) {
            return false;
        }
        committed_[commit.user_id] = commit.nonce_hash;
        if (committed_.size() == participant_keys_.size()) {
            phase_ = Phase::Reveal;
        }
        return true;
    }

    bool SessionVerification::process_reveal(const chain::GroupBroadcastNonceReveal& reveal) {
        if (phase_ != Phase::Reveal) {
            return false;
        }
        const auto it = participant_keys_.find(reveal.user_id);
        if (it == participant_keys_.end()) {
            return false;
        }
        if (!openssl::Key25519::verify(
                bytes::view(it->second),
                bytes::view(data_to_sign(reveal)),
                bytes::view(reveal.signature)
            )) {
            return false;
        }
        if (revealed_.contains(reveal.user_id)) {
            return false;
        }
        if (
            const auto committed_it = committed_.find(reveal.user_id);
            committed_it == committed_.end() || sha256(bytes::view(reveal.nonce)) != committed_it->second
        ) {
            return false;
        }
        revealed_[reveal.user_id] = reveal.nonce;
        if (revealed_.size() == participant_keys_.size()) {
            std::vector<tl::Hash256> nonces;
            nonces.reserve(revealed_.size());
            for (const auto& nonce : revealed_ | std::views::values) {
                nonces.push_back(nonce);
            }
            std::ranges::sort(nonces);
            bytes::binary full_nonce;
            full_nonce.reserve(nonces.size() * 32);
            for (const auto& nonce : nonces) {
                full_nonce.insert(full_nonce.end(), nonce.begin(), nonce.end());
            }
            const auto digest = openssl::Hmac::sha512(bytes::view(full_nonce), bytes::view(last_block_hash_));
            emoji_hash_value_ = bytes::binary(digest.begin(), digest.end());
            phase_ = Phase::End;
        }
        return true;
    }

    bool SessionVerification::process_broadcast(const chain::GroupBroadcast& broadcast) {
        tl::Hash256 broadcast_hash{};
        std::visit(
            [&broadcast_hash](const auto& value) {
                broadcast_hash = value.chain_hash;
            },
            broadcast.value
        );
        if (broadcast_hash != last_block_hash_) {
            return false;
        }
        return std::visit([this]<typename T>(const T& value) {
            if constexpr (std::is_same_v<T, chain::GroupBroadcastNonceCommit>) {
                return process_commit(value);
            } else {
                return process_reveal(value);
            }
        },
                          broadcast.value);
    }

    void SessionVerification::emit_reveal_if_needed() {
        if (phase_ == Phase::Reveal && !sent_reveal_) {
            sent_reveal_ = true;
            chain::GroupBroadcastNonceReveal reveal;
            reveal.user_id = self_user_id_;
            reveal.chain_height = height_;
            reveal.chain_hash = last_block_hash_;
            reveal.nonce = self_nonce_;
            reveal.signature = private_key_.sign(bytes::view(data_to_sign(reveal)));

            tl::TlWriter writer;
            reveal.store_boxed(writer);
            pending_outbound_.push_back(writer.result());
        }
    }

    bool SessionVerification::receive_inbound_message(const bytes::const_span message) {
        tl::TlReader reader(message);
        auto broadcast = chain::GroupBroadcast::fetch_boxed(reader);
        if (!reader.finish()) {
            return false;
        }
        int32_t chain_height = -1;
        std::visit([&chain_height](const auto& value) {
            chain_height = value.chain_height;
        },
                   broadcast.value);

        if (chain_height < height_) {
            return true;
        }
        if (chain_height > height_) {
            const auto data = message.data();
            delayed_[chain_height].emplace_back(data, data + message.size());
            return true;
        }
        const auto applied = process_broadcast(broadcast);
        emit_reveal_if_needed();
        return applied;
    }

    std::optional<bytes::binary> SessionVerification::emoji_hash() const {
        return emoji_hash_value_;
    }

    std::vector<bytes::binary> SessionVerification::pull_outbound_messages() {
        std::vector<bytes::binary> result;
        std::swap(result, pending_outbound_);
        return result;
    }

    void SessionVerification::on_new_main_block(const chain::Blockchain& blockchain) {
        self_nonce_ = random_nonce();
        height_ = blockchain.height();
        last_block_hash_ = blockchain.hash();
        phase_ = Phase::Commit;
        committed_.clear();
        revealed_.clear();
        emoji_hash_value_.reset();
        sent_reveal_ = false;

        participant_keys_.clear();
        for (const auto& participant : blockchain.current_group_state().participants) {
            participant_keys_.emplace(participant.user_id, participant.public_key);
        }

        chain::GroupBroadcastNonceCommit commit;
        commit.user_id = self_user_id_;
        commit.chain_height = height_;
        commit.chain_hash = last_block_hash_;
        commit.nonce_hash = sha256(bytes::view(self_nonce_));
        commit.signature = private_key_.sign(bytes::view(data_to_sign(commit)));

        tl::TlWriter writer;
        commit.store_boxed(writer);
        pending_outbound_.clear();
        pending_outbound_.push_back(writer.result());

        if (const auto it = delayed_.find(height_); it != delayed_.end()) {
            const auto pending = std::move(it->second);
            delayed_.erase(it);
            for (const auto& message : pending) {
                tl::TlReader reader(bytes::view(message));
                const auto broadcast = chain::GroupBroadcast::fetch_boxed(reader);
                if (!reader.finish()) {
                    process_broadcast(broadcast);
                }
            }
            emit_reveal_if_needed();
        }
    }
} // ntgcalls::e2e
