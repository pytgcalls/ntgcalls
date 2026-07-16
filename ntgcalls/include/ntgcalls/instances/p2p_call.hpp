//
// Created by Lauren on 15/03/24.
//

#pragma once
#include <ntgcalls/instances/call_interface.hpp>
#include <ntgcalls/instances/e2e_interface.hpp>
#include <ntgcalls/p2p/auth_params.hpp>
#include <ntgcalls/p2p/dh_config.hpp>
#include <ntgcalls/p2p/rtc_server.hpp>
#include <ntgcalls/signaling/signaling.hpp>

namespace ntgcalls::instances {

    class P2PCall final: public CallInterface, public E2EInterface {
        bytes::binary random_power_, prime_;
        std::optional<signaling::crypto::RawKey> key_;
        bytes::binary skip_exchange_key_;
        bool skip_is_outgoing_ = false;
        std::string fingerprint_emojis_;
        std::optional<bytes::binary> ga_hash_, g_a_or_b_;
        std::atomic_bool handshake_completed_ = false;
        std::shared_ptr<signaling::SignalingInterface> signaling_;
        wrtc::utils::synchronized_callback<void(bytes::binary)> on_emit_data_;
        wrtc::utils::synchronized_callback<void(std::string)> update_emojis_callback_;
        std::vector<wrtc::models::IceCandidate> pending_ice_candidates_;
        signaling::Signaling::Version protocol_version_ = signaling::Signaling::Version::Unknown;

        void process_signaling_data(const bytes::binary& buffer);

        void apply_pending_ice_candidates();

        void send_media_state(media::MediaState media_state) const;

        void send_offer_if_needed() const;

        void send_initial_setup() const;

    public:
        explicit P2PCall(wrtc::utils::SafeThread& update_thread): CallInterface(update_thread) {}

        void stop() override;

        void init() const;

        bytes::binary init_exchange(const p2p::DhConfig &dh_config, const std::optional<bytes::binary> &ga_hash);

        p2p::AuthParams exchange_keys(const bytes::binary &g_a_or_b, int64_t fingerprint);

        void skip_exchange(bytes::binary encryption_key, bool is_outgoing);

        void connect(const std::vector<p2p::RTCServer>& servers, const std::vector<std::string>& versions, bool p2p_allowed, const std::optional<std::string> &custom_parameters);

        Type type() const override;

        std::string get_fingerprint_emojis() override;

        void on_signaling_data(const std::function<void(const bytes::binary&)>& callback);

        void on_update_emojis(const std::function<void(std::string)>& callback) override;

        void send_signaling_data(const bytes::binary& buffer) const;
    };

} // ntgcalls
