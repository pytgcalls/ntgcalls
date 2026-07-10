//
// Created by Lauren on 29/03/24.
//

#pragma once
#include <p2p/client/relay_port_factory_interface.h>
#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/interfaces/network_interface.hpp>
#include <wrtc/interfaces/media/channels/outgoing_audio_channel.hpp>
#include <wrtc/models/connection_description.hpp>
#include <wrtc/interfaces/content_negotiation_context.hpp>
#include <wrtc/models/route_description.hpp>
#include <wrtc/models/rtc_server.hpp>
#include <wrtc/utils/json.hpp>

namespace wrtc::interfaces {

    class NativeConnection final : public NativeNetworkInterface {
        utils::json custom_parameters_;
        bool is_outgoing_, enable_p2p_;
        int64_t last_disconnected_timestamp_ = 0;
        std::vector<models::RTCServer> rtc_servers_;
        std::unique_ptr<webrtc::RelayPortFactoryInterface> relay_port_factory_;
        std::optional<models::RouteDescription> current_route_description_;
        std::optional<models::ConnectionDescription> current_connection_description_;
        std::unique_ptr<webrtc::RtcEventLogNull> event_log_;
        std::unique_ptr<ContentNegotiationContext> content_negotiation_context_;
        std::optional<std::string> audio_channel_id_, video_channel_id_;

        void notify_state_updated();

        void candidate_pair_changed(webrtc::CandidatePairChangeEvent const &event);

        void check_connection_timeout();

    protected:
        void start() override;

        bool get_custom_parameter_bool(const std::string& name) const override;

        static models::CandidateDescription connection_description_from_candidate(const webrtc::Candidate &candidate);

        webrtc::RelayPortFactoryInterface* get_relay_port_factory() override;

        std::pair<webrtc::ServerAddresses, std::vector<webrtc::RelayServerConfig>> get_stun_and_turn_servers() override;

        void set_port_allocator_flags(webrtc::BasicPortAllocator* port_allocator) override;

        webrtc::TimeDelta get_regather_on_failed_networks_interval() override;

        webrtc::IceRole ice_role() const override;

        webrtc::IceMode ice_mode() const override;

        void register_transport_callbacks(webrtc::P2PTransportChannel* transport_channel) override;

        std::optional<webrtc::SSLRole> dtls_role() const override;

        bool supports_renomination() const override;

        void state_updated(bool is_connected) override;

        int candidate_pool_size() const override;

        void rtp_packet_received(const webrtc::RtpPacketReceived& packet) override {}

        bool is_group_connection() const override;

    public:
        explicit NativeConnection(std::vector<models::RTCServer> rtc_servers, bool enable_p2p, bool is_outgoing, const utils::json& custom_parameters);

        void open() override;

        void close() override;

        void create_channels();

        void add_ice_candidate(const models::IceCandidate& raw_candidate) const override;

        void set_remote_params(models::PeerIceParameters remote_ice_parameters, std::unique_ptr<webrtc::SSLFingerprint> fingerprint, const std::string& ssl_setup);

        std::unique_ptr<ContentNegotiationContext::NegotiationContents> get_pending_offer() const;

        std::unique_ptr<ContentNegotiationContext::NegotiationContents> set_pending_answer(std::unique_ptr<ContentNegotiationContext::NegotiationContents> answer) const;

        std::unique_ptr<media::tracks::MediaTrackInterface> add_outgoing_track(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) override;
    };
} // wrtc::interfaces