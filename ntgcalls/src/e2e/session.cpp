//
// Created by Lauren on 17/06/26.
//

#include <api/units/time_delta.h>
#include <ntgcalls/e2e/session.hpp>
#include <ntgcalls/e2e/chain/client_blockchain.hpp>
#include <ntgcalls/e2e/chain/message_encryption.hpp>
#include <ntgcalls/utils/emoji_fingerprint.hpp>
#include <wrtc/utils/random.hpp>

namespace ntgcalls::e2e {
    Session::Session(wrtc::utils::SafeThread& update_thread, int64_t user_id): update_thread_(update_thread) {
        private_key_ = openssl::Key25519::generate();
        session_encryption_ = std::make_unique<SessionEncryption>(user_id, private_key_);
        session_verification_ = std::make_unique<SessionVerification>(user_id, private_key_);
    }

    Session::~Session() {
        const std::lock_guard lock(mutex_);
        session_encryption_ = nullptr;
        session_verification_ = nullptr;
    }

    bytes::array<32> Session::random_secret() {
        bytes::array<32> secret{};
        bytes::random_fill(bytes::span(secret.data(), secret.size()));
        return secret;
    }

    std::vector<chain::Change> Session::make_changes_for_new_state(const chain::GroupState& group_state) {
        const auto ephemeral_key = openssl::Key25519::generate();
        const auto group_shared_key = random_secret();
        const auto one_time_secret = random_secret();
        const auto encrypted_group_shared_key = chain::MessageEncryption::encrypt_data(
            bytes::view(group_shared_key),
            bytes::view(one_time_secret)
        );
        chain::SharedKey shared_key;
        shared_key.ek = ephemeral_key.public_key_bytes();
        shared_key.encrypted_shared_key.assign(
            reinterpret_cast<const char*>(encrypted_group_shared_key.data()),
            encrypted_group_shared_key.size()
        );
        for (const auto& participant : group_state.participants) {
            const auto shared_secret = ephemeral_key.compute_shared_secret(bytes::view(participant.public_key));
            const auto header = chain::MessageEncryption::encrypt_header(
                bytes::view(one_time_secret),
                bytes::view(encrypted_group_shared_key),
                bytes::view(shared_secret)
            );
            if (!header) {
                return {};
            }
            shared_key.dest_user_id.push_back(participant.user_id);
            shared_key.dest_header.push_back(*header);
        }
        return {
            chain::Change{
                chain::ChangeSetGroupState{group_state}
            },
            chain::Change{
                chain::ChangeSetSharedKey{std::move(shared_key)}
            }
        };
    }

    bool Session::is_good_magic(const int32_t magic) {
        return magic == chain::Block::kID || magic == chain::GroupBroadcastNonceCommit::kID || magic == chain::GroupBroadcastNonceReveal::kID;
    }

    int32_t Session::read_magic(const bytes::const_span block) {
        const auto data = block.data();
        return static_cast<int32_t>(
            static_cast<uint32_t>(data[0]) |
            static_cast<uint32_t>(data[1]) << 8 |
            static_cast<uint32_t>(data[2]) << 16 |
            static_cast<uint32_t>(data[3]) << 24
        );
    }

    void Session::write_magic(bytes::binary& block, const int32_t magic) {
        const auto raw = static_cast<uint32_t>(magic);
        block[0] = static_cast<uint8_t>(raw & 0xff);
        block[1] = static_cast<uint8_t>(raw >> 8 & 0xff);
        block[2] = static_cast<uint8_t>(raw >> 16 & 0xff);
        block[3] = static_cast<uint8_t>(raw >> 24 & 0xff);
    }

    std::optional<bytes::binary> Session::from_server_to_local(const bytes::const_span block) {
        if (block.size() < 4) {
            return std::nullopt;
        }
        const auto server_magic = read_magic(block);
        if (is_good_magic(server_magic)) {
            return std::nullopt;
        }
        bytes::binary result((block.data()), block.data() + block.size());
        write_magic(result, server_magic - 1);
        return result;
    }

    std::optional<bytes::binary> Session::create_zero_block(
        const openssl::Key25519& key,
        const chain::GroupState& group_state
    ) {
        const auto blockchain = chain::ClientBlockchain::create_empty();
        const auto changes = make_changes_for_new_state(group_state);
        if (changes.empty()) {
            return std::nullopt;
        }
        return blockchain.build_block(changes, key);
    }

    std::optional<bytes::binary> Session::create_self_add_block(
        const openssl::Key25519& key,
        const bytes::const_span previous_server_block,
        const chain::GroupParticipant& self
    ) {
        const auto previous = from_server_to_local(previous_server_block);
        if (!previous) {
            return std::nullopt;
        }
        const auto blockchain = chain::ClientBlockchain::create_from_block(bytes::view(*previous));
        if (!blockchain) {
            return std::nullopt;
        }
        auto state = blockchain->group_state();
        std::erase_if(state.participants, [&self](const chain::GroupParticipant& p) {
            return p.user_id == self.user_id;
        });
        state.participants.push_back(self);
        const auto changes = make_changes_for_new_state(state);
        if (changes.empty()) {
            return std::nullopt;
        }
        return blockchain->build_block(changes, key);
    }

    std::optional<bytes::binary> Session::receive_inbound_message(const bytes::const_span server_message) {
        const std::lock_guard lock(mutex_);
        const auto local = from_server_to_local(server_message);
        if (!local) {
            return std::nullopt;
        }
        session_verification_->receive_inbound_message(bytes::view(*local));
        return session_verification_->emoji_hash();
    }

    void Session::apply(const int subchain, const bytes::binary& last) {
        if (subchain) {
            if (!blockchain_) {
                failed_ = true;
                return;
            }
            update_emojis(receive_inbound_message(bytes::view(last)));
            check_for_outbound_messages();
        } else if (blockchain_) {
            if (!apply_block(bytes::view(last))) {
                failed_ = true;
                return;
            }
            refresh_from_call();
            check_for_outbound_messages();
        } else {
            if (!init_blockchain(bytes::view(last))) {
                failed_ = true;
                return;
            }
            for (auto i = 0; i != kSubChainsCount; ++i) {
                if (!subchains_[i].waiting_active) {
                    schedule_short_poll(i);
                }
            }
            refresh_from_call();
            check_for_outbound_messages();
        }
    }

    void Session::schedule_waiting(const int subchain) {
        auto& entry = subchains_[subchain];
        const auto generation = ++entry.waiting_generation;
        entry.waiting_active = true;
        const std::weak_ptr weak(shared_from_this());
        update_thread_.PostDelayedTask(
            [weak, subchain, generation] {
                const auto strong = weak.lock();
                if (!strong) {
                    return;
                }
                auto& state = strong->subchains_[subchain];
                if (state.waiting_generation != generation) {
                    return;
                }
                state.waiting_active = false;
                strong->check_waiting_blocks(subchain, true);
            },
            webrtc::TimeDelta::Millis(kShortPollWaitForMs)
        );
    }

    void Session::cancel_waiting(const int subchain) {
        auto& entry = subchains_[subchain];
        ++entry.waiting_generation;
        entry.waiting_active = false;
    }

    void Session::cancel_short_poll(const int subchain) {
        ++subchains_[subchain].short_poll_generation;
    }

    void Session::check_waiting_blocks(const int subchain, const bool waited) {
        if (failed_) {
            return;
        }

        auto& entry = subchains_[subchain];
        if (!blockchain_) {
            schedule_waiting(subchain);
            return;
        }
        if (entry.short_polling) {
            return;
        }
        auto& waiting = entry.waiting;
        cancel_short_poll(subchain);
        while (!waiting.empty()) {
            const auto index = waiting.begin()->first;
            if (index > entry.height) {
                if (waited) {
                    short_poll(subchain);
                } else {
                    schedule_waiting(subchain);
                }
                return;
            }
            if (index == entry.height) {
                apply(subchain, waiting.begin()->second);
                if (failed_) {
                    return;
                }
                entry.height = std::max(entry.height, index + 1);
            }
            waiting.erase(waiting.begin());
        }
        cancel_waiting(subchain);
        schedule_short_poll(subchain);
    }

    void Session::check_for_outbound_messages() {
        if (!blockchain_) {
            return;
        }
        if (const auto messages = pull_outbound_messages(); !messages.empty()) {
            (void) outbound_block_callback_(messages.back());
        }
    }

    std::vector<bytes::binary> Session::pull_outbound_messages() {
        const std::lock_guard lock(mutex_);
        return session_verification_->pull_outbound_messages();
    }

    bool Session::init_blockchain(const bytes::const_span server_block) {
        const std::lock_guard lock(mutex_);
        const auto local = from_server_to_local(server_block);
        if (!local) {
            failed_ = true;
            return false;
        }
        auto created = chain::ClientBlockchain::create_from_block(bytes::view(*local));
        if (!created) {
            failed_ = true;
            return false;
        }
        blockchain_ = std::move(*created);
        session_verification_->on_new_main_block(blockchain_->inner());
        return update_group_shared_key();
    }

    bool Session::apply_block(const bytes::const_span server_block) {
        const std::lock_guard lock(mutex_);
        const auto local = from_server_to_local(server_block);
        if (!local) {
            failed_ = true;
            return false;
        }
        if (!blockchain_->try_apply_block(bytes::view(*local))) {
            failed_ = true;
            return false;
        }
        session_verification_->on_new_main_block(blockchain_->inner());
        return update_group_shared_key();
    }

    bool Session::update_group_shared_key() {
        session_encryption_->forget_shared_key(blockchain_->height() - 1, blockchain_->previous_block_hash());

        const auto group_state = blockchain_->group_state();
        if (
            const auto self = State::find_participant(group_state, private_key_.public_key_bytes());
            !self || self->user_id != session_encryption_->user_id()
        ) {
            failed_ = true;
            return false;
        }
        auto secret = decrypt_shared_key();
        if (!secret) {
            failed_ = true;
            return false;
        }
        if (State::group_state_version(group_state) >= 1) {
            const auto rehashed = openssl::Hmac::sha512(
                bytes::view(*secret),
                bytes::view(blockchain_->last_block_hash())
            );
            secret = bytes::binary(rehashed.begin(), rehashed.begin() + 32);
        }
        return session_encryption_->add_shared_key(blockchain_->height(), blockchain_->last_block_hash(), *secret, group_state);
    }

    std::optional<bytes::binary> Session::decrypt_shared_key() const {
        const auto& [ek, encryptedSharedKey, destUserId, destHeader] = blockchain_->group_shared_key();
        for (size_t i = 0; i < destUserId.size(); ++i) {
            if (destUserId[i] != session_encryption_->user_id()) {
                continue;
            }
            const auto shared_secret = private_key_.compute_shared_secret(bytes::view(ek));
            const auto one_time_secret = chain::MessageEncryption::decrypt_header(
                bytes::view(destHeader[i]),
                bytes::view(encryptedSharedKey),
                bytes::view(shared_secret)
            );
            if (!one_time_secret) {
                return std::nullopt;
            }
            auto decrypted = chain::MessageEncryption::decrypt_data(
                bytes::view(encryptedSharedKey),
                bytes::view(*one_time_secret)
            );
            if (!decrypted || decrypted->size() != 32) {
                return std::nullopt;
            }
            return decrypted;
        }
        return std::nullopt;
    }

    void Session::schedule_short_poll(const int subchain) {
        auto& entry = subchains_[subchain];
        const auto generation = ++entry.short_poll_generation;
        const std::weak_ptr weak = weak_from_this();
        update_thread_.PostDelayedTask(
            [weak, subchain, generation] {
                const auto strong = weak.lock();
                if (!strong || strong->subchains_[subchain].short_poll_generation != generation) {
                    return;
                }
                strong->short_poll(subchain);
            },
            webrtc::TimeDelta::Millis(kShortPollTimeoutMs)
        );
    }

    chain::GroupState Session::current_group_state() {
        const std::lock_guard lock(mutex_);
        return blockchain_->group_state();
    }

    std::optional<bytes::binary> Session::emoji_hash() {
        const std::lock_guard lock(mutex_);
        return session_verification_->emoji_hash();
    }

    void Session::refresh_from_call() {
        if (!blockchain_) {
            return;
        }
        std::unordered_set<int64_t> user_ids;
        for (const auto& participant : current_group_state().participants) {
            user_ids.insert(participant.user_id);
        }
        update_emojis(emoji_hash());
    }

    void Session::update_emojis(const std::optional<bytes::binary>& hash) {
        if (hash) {
            fingerprint_emojis_ = utils::EmojiFingerprint::from_hash(bytes::view(*hash));
            (void) update_emojis_callback_(fingerprint_emojis_);
        }
    }

    std::string Session::get_fingerprint_emojis() {
        const std::lock_guard lock(mutex_);
        return fingerprint_emojis_;
    }

    void Session::set_last_block(const bytes::binary& block) {
        last_block_ = block;
    }

    bytes::binary Session::make_join_block() {
        if (failed_) return {};

        chain::GroupParticipant self;
        self.user_id = session_encryption_->user_id();
        self.add_users = true;
        self.remove_users = true;
        self.public_key = private_key_.public_key_bytes();
        self.version = 0;

        std::optional<bytes::binary> block;
        if (last_block_) {
            block = create_self_add_block(
                private_key_,
                bytes::view(last_block_.value()),
                self
            );
        } else {
            chain::GroupState group_state;
            group_state.participants = {self};
            group_state.external_permissions = Permissions::AddUsers | Permissions::RemoveUsers;
            block = create_zero_block(private_key_, group_state);
        }
        if (!block) {
            failed_ = true;
            return {};
        }
        return block.value();
    }

    void Session::short_poll(const int subchain) {
        auto& entry = subchains_[subchain];
        cancel_waiting(subchain);
        cancel_short_poll(subchain);
        if (subchain && !blockchain_) {
            schedule_waiting(subchain);
            return;
        }
        entry.short_polling = true;
        (void) subchain_request_callback_(
            SubchainRequest{
                subchain,
                entry.height,
                kShortPollChainBlocksPerRequest
            }
        );
    }

    void Session::finish_subchain_request(const int subchain) {
        if (failed_) {
            return;
        }
        subchains_[subchain].short_polling = false;
        check_waiting_blocks(subchain, false);
    }

    bytes::binary Session::public_key() const {
        const auto public_key = private_key_.public_key_bytes();
        return {public_key.begin(), public_key.end()};
    }

    void Session::apply_blocks(
        const int subchain,
        const int next_offset,
        const std::vector<bytes::binary>& blocks,
        const bool from_short_poll
    ) {
        if (!subchain && !blocks.empty() && next_offset > last_block_height_) {
            last_block_ = blocks.back();
            last_block_height_ = next_offset;
        }

        auto& entry = subchains_[subchain];
        if (from_short_poll) {
            auto i = entry.waiting.begin();
            while (i != entry.waiting.end() && i->first < next_offset) {
                ++i;
            }
            entry.waiting.erase(entry.waiting.begin(), i);

            if (subchain && !blockchain_ && !blocks.empty()) {
                RTC_LOG(LS_ERROR) << "ConferenceCall: broadcast short-poll block before the call was created";
                failed_ = true;
                return;
            }
        } else {
            entry.last_update = webrtc::TimeMillis();
        }
        if (failed_) {
            return;
        }

        if (auto index = next_offset - static_cast<int>(blocks.size()); !from_short_poll && (index > entry.height || (!blockchain_ && subchain))) {
            for (const auto& block : blocks) {
                entry.waiting.emplace(index++, block);
            }
        } else {
            if (from_short_poll && subchain && index > entry.height) {
                entry.height = index;
            }
            for (const auto& block : blocks) {
                if (!blockchain_ || entry.height == index) {
                    apply(subchain, block);
                }
                entry.height = std::max(entry.height, ++index);
            }
            entry.height = std::max(entry.height, next_offset);
        }
        check_waiting_blocks(subchain);
    }

    void Session::on_outbound_block(const std::function<void(bytes::binary)>& callback) {
        outbound_block_callback_ = callback;
    }

    void Session::on_subchain_request(const std::function<void(SubchainRequest)>& callback) {
        subchain_request_callback_ = callback;
    }

    void Session::on_update_emoji_hash(const std::function<void(std::string)>& callback) {
        update_emojis_callback_ = callback;
    }

    bytes::binary Session::encrypt(const bytes::binary& data, const size_t unencrypted_prefix) {
        const std::lock_guard lock(mutex_);
        if (failed_) return {};
        const auto r = session_encryption_->encrypt(0, bytes::view(data), unencrypted_prefix);
        return r ? *r : bytes::binary();
    }

    bytes::binary Session::decrypt(const int64_t user_id, const bytes::binary& data) {
        const std::lock_guard lock(mutex_);
        if (failed_) return {};
        const auto r = session_encryption_->decrypt(user_id, bytes::view(data));
        return r ? *r : bytes::binary();
    }
} // ntgcalls::e2e
