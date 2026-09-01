//
// Created by Lauren on 01/10/24.
//

#pragma once

#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/interfaces/network_interface.hpp>
#include <wrtc/interfaces/mtproto/mtproto_stream.hpp>
#include <wrtc/interfaces/response_payload.hpp>
#include <wrtc/models/ssrc_mapping.hpp>

namespace wrtc::interfaces {

    class GroupConnection final: public NativeNetworkInterface {
        int64_t last_network_activity_ms_ = 0;
        uint32_t outgoing_audio_ssrc_ = 0, outgoing_video_ssrc_ = 0;
        std::vector<models::SsrcGroup> outgoing_video_ssrc_groups_;
        bool is_presentation_ = false, is_conference_ = false;
        bool is_rtc_connected_ = false, is_stream_connected_ = false;
        bool last_effectively_connected_ = false;
        ConnectionMode connection_mode_ = ConnectionMode::None;
        ResponsePayload::Media media_config_;
        std::shared_ptr<mtproto::MTProtoStream> mtproto_stream_;
        std::map<uint32_t, int64_t> audio_ssrc_to_user_id_;
        std::unordered_set<uint32_t> pending_audio_ssrcs_;
        utils::synchronized_callback<void()> request_participants_callback_;

    protected:
        bool supports_renomination() const override;

        webrtc::IceRole ice_role() const override;

        webrtc::IceMode ice_mode() const override;

        std::optional<webrtc::SSLRole> dtls_role() const override;

        std::pair<webrtc::ServerAddresses, std::vector<webrtc::RelayServerConfig>> get_stun_and_turn_servers() override;

        webrtc::RelayPortFactoryInterface* get_relay_port_factory() override;

        void register_transport_callbacks(webrtc::P2PTransportChannel* transport_channel) override;

        webrtc::TimeDelta get_regather_on_failed_networks_interval() override;

        bool get_custom_parameter_bool(const std::string& name) const override;

        void set_port_allocator_flags(webrtc::BasicPortAllocator* port_allocator) override;

        void start() override;

        void restart_data_channel();

        void generate_ssrcs();

        void state_updated(bool is_connected) override;

        int candidate_pool_size() const override;

        void update_is_connected();

        void rtp_packet_received(const webrtc::RtpPacketReceived& packet) override;

        void add_incoming_audio(int64_t user_id, uint32_t ssrc, const std::string& endpoint);

        void begin_audio_channel_cleanup_timer();

        bool is_group_connection() const override;

    public:
        explicit GroupConnection(bool is_presentation, bool is_conference);

        std::string get_join_payload();

        void add_ice_candidate(const models::IceCandidate& raw_candidate) const override;

        void set_remote_params(models::PeerIceParameters remote_ice_parameters, std::unique_ptr<webrtc::SSLFingerprint> fingerprint);

        void connect_media_stream();

        void set_connection_mode(ConnectionMode kind);

        void send_broadcast_part(int64_t segment_id, int32_t part_id, models::MediaSegment::Part::Status status, bool quality_update, const std::optional<bytes::binary>& data) const;

        void on_request_broadcast_part(const std::function<void(models::SegmentPartRequest)>& callback) const;

        void send_broadcast_timestamp(int64_t timestamp) const;

        void on_request_broadcast_timestamp(const std::function<void()>& callback) const;

        void create_channels(const ResponsePayload::Media& media);

        void update_audio_ssrc_mappings(const std::vector<models::SsrcMapping>& audio_ssrcs);

        uint32_t add_incoming_video(int64_t user_id, const std::string& endpoint, const std::vector<models::SsrcGroup>& ssrc_groups);

        bool remove_incoming_video(const std::string& endpoint);

        void on_request_participants(const std::function<void()>& callback);

        void set_e2e_encryptor(media::E2EEncryptor* encryptor);

        void open() override;

        void close() override;

        ResponsePayload::Media get_media_config() const;

        ConnectionMode get_connection_mode() const override;

        void enable_audio_incoming(bool enable) override;

        void enable_video_incoming(bool enable, bool is_screen_cast) override;
    };

} // wrtc::interfaces
