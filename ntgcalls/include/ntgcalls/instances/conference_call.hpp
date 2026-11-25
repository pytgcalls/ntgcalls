//
// Created by Lauren on 11/06/26.
//

#pragma once
#include <mutex>
#include <ntgcalls/e2e/session.hpp>
#include <ntgcalls/instances/e2e_interface.hpp>
#include <ntgcalls/instances/group_call.hpp>
#include <ntgcalls/p2p/conference_join_params.hpp>

namespace ntgcalls::instances {

    class ConferenceCall final: public GroupCall, public E2EInterface {
        int64_t user_id_ = 0;
        std::shared_ptr<e2e::Session> session_;
        mutable std::mutex session_mutex_;
        wrtc::utils::synchronized_callback<void(wrtc::models::ParticipantsRequest)> request_participants_callback_;
        wrtc::utils::synchronized_callback<void(bytes::binary)> outbound_block_callback_;
        wrtc::utils::synchronized_callback<void(e2e::SubchainRequest)> subchain_request_callback_;
        wrtc::utils::synchronized_callback<void(std::string)> update_emojis_callback_;
        wrtc::utils::synchronized_callback<void()> e2e_failure_callback_;
        wrtc::utils::synchronized_callback<void(std::vector<int64_t>)> e2e_participants_callback_;

        std::shared_ptr<e2e::Session> session() const;

        void create_session(const std::optional<bytes::binary>& last_block);

        p2p::ConferenceJoinParams join_params(std::string payload) const;

    protected:
        void on_connection_opened(wrtc::interfaces::GroupConnection* conn) override;

    public:
        explicit ConferenceCall(wrtc::utils::SafeThread& update_thread): GroupCall(update_thread) {}

        p2p::ConferenceJoinParams init_conference(int64_t user_id, const std::optional<bytes::binary>& last_block);

        p2p::ConferenceJoinParams rejoin_conference(const std::optional<bytes::binary>& last_block);

        std::string init_presentation() override;

        void connect(const std::string& json_data, bool is_presentation) override;

        void migrate(P2PCall* p2p_call);

        void apply_blocks(
            int subchain,
            int next_offset,
            const std::vector<bytes::binary>& blocks,
            bool from_short_poll
        ) const;

        void update_audio_ssrc_mappings(const std::vector<wrtc::models::SsrcMapping>& audio_ssrcs) const;

        void finish_subchain_request(int subchain) const;

        void on_outbound_block(const std::function<void(bytes::binary)>& callback);

        void on_subchain_request(const std::function<void(e2e::SubchainRequest)>& callback);

        void on_request_participants(const std::function<void(wrtc::models::ParticipantsRequest)>& callback);

        void on_update_emojis(const std::function<void(std::string)>& callback) override;

        void on_e2e_failure(const std::function<void()>& callback);

        void on_e2e_participants(const std::function<void(std::vector<int64_t>)>& callback);

        bytes::binary create_remove_block(const std::vector<int64_t>& user_ids) const;

        std::vector<int64_t> get_e2e_participants() const;

        Type type() const override;

        void stop() override;

        std::string get_fingerprint_emojis() override;

        bytes::binary encrypt_message(const bytes::binary& data) const;

        bytes::binary decrypt_message(int64_t user_id, const bytes::binary& data) const;
    };
} // ntgcalls::instances
