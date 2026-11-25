//
// Created by Lauren on 11/06/26.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/instances/conference_call.hpp>

namespace ntgcalls::instances {
    p2p::ConferenceJoinParams ConferenceCall::init_conference(const int64_t user_id, const std::optional<bytes::binary>& last_block) {
        user_id_ = user_id;
        create_session(last_block);
        return join_params(init());
    }

    p2p::ConferenceJoinParams ConferenceCall::rejoin_conference(const std::optional<bytes::binary>& last_block) {
        const auto previous = session();
        create_session(last_block);
        return join_params(rejoin());
    }

    p2p::ConferenceJoinParams ConferenceCall::join_params(std::string payload) const {
        const auto current = session();
        return {
            std::move(payload),
            current->public_key(),
            current->make_join_block()
        };
    }

    std::shared_ptr<e2e::Session> ConferenceCall::session() const {
        const std::lock_guard lock(session_mutex_);
        return session_;
    }

    void ConferenceCall::create_session(const std::optional<bytes::binary>& last_block) {
        auto created = std::make_shared<e2e::Session>(update_thread_, user_id_);
        if (last_block) {
            created->set_last_block(*last_block);
        }
        const std::weak_ptr weak(shared_from_this());
        created->on_outbound_block([weak](const bytes::binary& block) {
            if (const auto strong = std::static_pointer_cast<ConferenceCall>(weak.lock())) {
                (void) strong->outbound_block_callback_(block);
            }
        });
        created->on_subchain_request([weak](const e2e::SubchainRequest request) {
            if (const auto strong = std::static_pointer_cast<ConferenceCall>(weak.lock())) {
                (void) strong->subchain_request_callback_(request);
            }
        });
        created->on_update_emoji_hash([weak](const std::string& emojis) {
            if (const auto strong = std::static_pointer_cast<ConferenceCall>(weak.lock())) {
                (void) strong->update_emojis_callback_(emojis);
            }
        });
        created->on_failure([weak] {
            if (const auto strong = std::static_pointer_cast<ConferenceCall>(weak.lock())) {
                (void) strong->e2e_failure_callback_();
            }
        });
        created->on_participants_change([weak](const std::vector<int64_t>& participants) {
            if (const auto strong = std::static_pointer_cast<ConferenceCall>(weak.lock())) {
                (void) strong->e2e_participants_callback_(participants);
            }
        });
        std::shared_ptr<e2e::Session> previous;
        {
            const std::lock_guard lock(session_mutex_);
            previous = std::exchange(session_, std::move(created));
        }
        if (previous) {
            previous->on_outbound_block(nullptr);
            previous->on_subchain_request(nullptr);
            previous->on_update_emoji_hash(nullptr);
            previous->on_failure(nullptr);
            previous->on_participants_change(nullptr);
        }
    }

    void ConferenceCall::on_connection_opened(wrtc::interfaces::GroupConnection* conn) {
        conn->set_e2e_encryptor(session().get());
        const std::weak_ptr weak(shared_from_this());
        conn->on_request_participants([weak](wrtc::models::ParticipantsRequest request) {
            const auto strong = std::static_pointer_cast<ConferenceCall>(weak.lock());
            if (!strong) {
                return;
            }
            (void) strong->request_participants_callback_(std::move(request));
        });
    }

    std::string ConferenceCall::init_presentation() {
        auto res = GroupCall::init_presentation();
        if (const auto presentation = presentation_connection()) {
            presentation->set_e2e_encryptor(session().get());
        }
        return res;
    }

    void ConferenceCall::connect(const std::string& json_data, const bool is_presentation) {
        GroupCall::connect(json_data, is_presentation);
        const auto current = session();
        current->short_poll(0);
        current->short_poll(1);
    }

    void ConferenceCall::migrate(P2PCall* p2p_call) {
        stream_manager_ = p2p_call->release_stream_manager();
        stream_manager_->enable_video_simulcast(true);
        stream_manager_->detach();
    }

    void ConferenceCall::apply_blocks(
        const int subchain,
        const int next_offset,
        const std::vector<bytes::binary>& blocks,
        const bool from_short_poll
    ) const {
        session()->apply_blocks(subchain, next_offset, blocks, from_short_poll);
    }

    void ConferenceCall::finish_subchain_request(const int subchain) const {
        session()->finish_subchain_request(subchain);
    }

    void ConferenceCall::update_audio_ssrc_mappings(const std::vector<wrtc::models::SsrcMapping>& audio_ssrcs) const {
        const auto current = connection();
        if (!current) {
            throw ConnectionError("Conference connection not initialized");
        }
        safe<wrtc::interfaces::GroupConnection>(current)->update_audio_ssrc_mappings(audio_ssrcs);
    }

    void ConferenceCall::on_outbound_block(const std::function<void(bytes::binary)>& callback) {
        outbound_block_callback_ = callback;
    }

    void ConferenceCall::on_subchain_request(const std::function<void(e2e::SubchainRequest)>& callback) {
        subchain_request_callback_ = callback;
    }

    void ConferenceCall::on_request_participants(const std::function<void(wrtc::models::ParticipantsRequest)>& callback) {
        request_participants_callback_ = callback;
    }

    void ConferenceCall::on_update_emojis(const std::function<void(std::string)>& callback) {
        update_emojis_callback_ = callback;
    }

    void ConferenceCall::on_e2e_failure(const std::function<void()>& callback) {
        e2e_failure_callback_ = callback;
    }

    void ConferenceCall::on_e2e_participants(const std::function<void(std::vector<int64_t>)>& callback) {
        e2e_participants_callback_ = callback;
    }

    bytes::binary ConferenceCall::create_remove_block(const std::vector<int64_t>& user_ids) const {
        auto block = session()->make_remove_block(user_ids);
        if (block.empty()) {
            throw CryptoError("Failed to create the remove block");
        }
        return block;
    }

    std::vector<int64_t> ConferenceCall::get_e2e_participants() const {
        return session()->participants();
    }

    CallInterface::Type ConferenceCall::type() const {
        return Type::Conference;
    }

    void ConferenceCall::stop() {
        GroupCall::stop();
        outbound_block_callback_ = nullptr;
        subchain_request_callback_ = nullptr;
        update_emojis_callback_ = nullptr;
        e2e_failure_callback_ = nullptr;
        e2e_participants_callback_ = nullptr;
    }

    std::string ConferenceCall::get_fingerprint_emojis() {
        return session()->get_fingerprint_emojis();
    }

    bytes::binary ConferenceCall::encrypt_message(const bytes::binary& data) const {
        auto encrypted = session()->encrypt(data, 0);
        if (encrypted.empty()) {
            throw CryptoError("Failed to encrypt the message");
        }
        return encrypted;
    }

    bytes::binary ConferenceCall::decrypt_message(const int64_t user_id, const bytes::binary& data) const {
        auto decrypted = session()->decrypt(user_id, data);
        if (decrypted.empty()) {
            throw CryptoError("Failed to decrypt the message");
        }
        return decrypted;
    }
} // ntgcalls::instances
