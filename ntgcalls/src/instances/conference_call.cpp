//
// Created by Lauren on 11/06/26.
//

#include <ntgcalls/instances/conference_call.hpp>

namespace ntgcalls::instances {
    p2p::ConferenceJoinParams ConferenceCall::init_conference(const int64_t user_id, const std::optional<bytes::binary>& last_block) {
        session_ = std::make_shared<e2e::Session>(update_thread_, user_id);
        auto payload = init();
        safe<wrtc::interfaces::GroupConnection>(connection_)->set_e2e_encryptor(session_.get());
        if (last_block) {
            session_->set_last_block(*last_block);
        }
        std::weak_ptr weak(shared_from_this());
        safe<wrtc::interfaces::GroupConnection>(connection_)->on_request_participants([weak] {
            const auto strong = std::static_pointer_cast<ConferenceCall>(weak.lock());
            if (!strong) {
                return;
            }
            (void) strong->request_participants_callback_();
        });
        return {
            std::move(payload),
            session_->public_key(),
            session_->make_join_block()
        };
    }

    std::string ConferenceCall::init_presentation() {
        auto res = GroupCall::init_presentation();
        presentation_connection_->set_e2e_encryptor(session_.get());
        return res;
    }

    void ConferenceCall::connect(const std::string& json_data, const bool is_presentation) {
        GroupCall::connect(json_data, is_presentation);
        session_->short_poll(0);
        session_->short_poll(1);
    }

    void ConferenceCall::migrate(const P2PCall* p2p_call) {
        stream_manager_ = std::move(p2p_call->stream_manager());
        stream_manager_->enable_video_simulcast(true);
        stream_manager_->detach();
    }

    void ConferenceCall::apply_blocks(
        const int subchain,
        const int next_offset,
        const std::vector<bytes::binary>& blocks,
        const bool from_short_poll
    ) const {
        session_->apply_blocks(subchain, next_offset, blocks, from_short_poll);
    }

    void ConferenceCall::finish_subchain_request(const int subchain) const {
        session_->finish_subchain_request(subchain);
    }

    void ConferenceCall::update_audio_ssrc_mappings(const std::vector<wrtc::models::SsrcMapping>& audio_ssrcs) const {
        const auto groupConnection = safe<wrtc::interfaces::GroupConnection>(connection_);
        if (!groupConnection) {
            throw ConnectionError("Conference connection not initialized");
        }
        groupConnection->update_audio_ssrc_mappings(audio_ssrcs);
    }

    void ConferenceCall::on_outbound_block(const std::function<void(bytes::binary)>& callback) const {
        session_->on_outbound_block(callback);
    }

    void ConferenceCall::on_subchain_request(const std::function<void(e2e::SubchainRequest)>& callback) const {
        session_->on_subchain_request(callback);
    }

    void ConferenceCall::on_request_participants(const std::function<void()>& callback) {
        request_participants_callback_ = callback;
    }

    void ConferenceCall::on_update_emojis(const std::function<void(std::string)>& callback) {
        session_->on_update_emoji_hash(callback);
    }

    CallInterface::Type ConferenceCall::type() const {
        return Type::Conference;
    }

    void ConferenceCall::stop() {
        GroupCall::stop();
        session_ = nullptr;
    }

    std::string ConferenceCall::get_fingerprint_emojis() {
        return session_->get_fingerprint_emojis();
    }
} // ntgcalls::instances
