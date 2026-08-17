//
// Created by Lauren on 01/10/24.
//

#pragma once

#include <api/scoped_refptr.h>
#include <p2p/base/dtls_transport.h>
#include <p2p/base/p2p_transport_channel.h>
#include <p2p/client/basic_port_allocator.h>
#include <pc/dtls_srtp_transport.h>
#include <pc/sdp_payload_type_suggester.h>
#include <rtc_base/rtc_certificate.h>
#include <wrtc/interfaces/network_interface.hpp>
#include <wrtc/interfaces/sctp_data_channel_provider_interface_impl.hpp>
#include <wrtc/interfaces/media/channel_manager.hpp>
#include <wrtc/interfaces/media/channels/incoming_audio_channel.hpp>
#include <wrtc/interfaces/media/channels/incoming_video_channel.hpp>
#include <wrtc/interfaces/media/channels/outgoing_audio_channel.hpp>
#include <wrtc/interfaces/media/channels/outgoing_video_channel.hpp>
#include <wrtc/models/peer_ice_parameters.hpp>

namespace wrtc::interfaces {

    class NativeNetworkInterface: public NetworkInterface, public std::enable_shared_from_this<NativeNetworkInterface> {
        struct H264FormatParameters {
            std::string profile_level_id;
            std::string packetization_mode;
            std::string level_assymetry_allowed;
        };

        std::unique_ptr<webrtc::BasicPortAllocator> port_allocator_;
        webrtc::scoped_refptr<webrtc::RTCCertificate> local_certificate_;
        std::unique_ptr<webrtc::AsyncDnsResolverFactoryInterface> async_resolver_factory_;

        void dtls_ready_to_send(bool is_ready_to_send);

        void reset_dtls_srtp_transport();

        void update_aggregate_states_n();

        static std::vector<webrtc::SdpVideoFormat> filter_supported_video_formats(std::vector<webrtc::SdpVideoFormat> const &formats);

        static H264FormatParameters parse_h264_format_parameters(webrtc::SdpVideoFormat const &format);

        static int get_h264_profile_level_id_priority(const std::string &profile_level_id);

        static int get_h264_packetization_mode_priority(const std::string &packetization_mode);

        static int get_h264_level_assymetry_allowed_priority(const std::string &level_assymetry_allowed);

    protected:
        std::mutex mutex_;
        std::unique_ptr<webrtc::Call> call_;
        std::unique_ptr<webrtc::SdpPayloadTypeSuggester> payload_type_suggester_;
        webrtc::LocalAudioSinkAdapter audio_sink_;
        media::LocalVideoAdapter video_sink_;
        std::weak_ptr<media::RemoteAudioSink> remote_audio_sink_;
        std::weak_ptr<media::RemoteVideoSink> remote_video_sink_;
        std::weak_ptr<media::RemoteVideoSink> remote_screen_cast_sink_;
        std::unique_ptr<media::ChannelManager> channel_manager_;
        std::unique_ptr<media::channels::OutgoingAudioChannel> audio_channel_;
        std::unique_ptr<media::channels::OutgoingVideoChannel> video_channel_;
        models::PeerIceParameters local_parameters_, remote_parameters_;
        webrtc::SocketFactory* underlying_socket_factory_ = nullptr;
        std::unique_ptr<webrtc::DtlsTransportInternal> dtls_transport_;
        std::unique_ptr<webrtc::DtlsSrtpTransport> dtls_srtp_transport_;
        std::unique_ptr<webrtc::P2PTransportChannel> transport_channel_;
        std::vector<webrtc::SdpVideoFormat> available_video_formats_;
        media::E2EEncryptor* encryptor_ = nullptr;
        std::map<int32_t, media::FrameTransformer::PayloadType> payload_type_mapping_;
        std::unique_ptr<SctpDataChannelProviderInterfaceImpl> data_channel_interface_;
        std::map<std::string, std::unique_ptr<media::channels::IncomingAudioChannel>> incoming_audio_channels_;
        std::map<std::string, std::unique_ptr<media::channels::IncomingVideoChannel>> incoming_video_channels_;
        std::map<std::string, models::MediaContent> pending_content_;
        bool connected_ = false, failed_ = false;

        virtual std::pair<webrtc::ServerAddresses, std::vector<webrtc::RelayServerConfig>> get_stun_and_turn_servers() = 0;

        virtual webrtc::RelayPortFactoryInterface* get_relay_port_factory() = 0;

        virtual void set_port_allocator_flags(webrtc::BasicPortAllocator* port_allocator) = 0;

        virtual webrtc::TimeDelta get_regather_on_failed_networks_interval() = 0;

        virtual bool get_custom_parameter_bool(const std::string& name) const = 0;

        virtual bool supports_renomination() const = 0;

        virtual webrtc::IceRole ice_role() const = 0;

        virtual webrtc::IceMode ice_mode() const = 0;

        virtual int candidate_pool_size() const = 0;

        virtual bool is_group_connection() const = 0;

        virtual std::optional<webrtc::SSLRole> dtls_role() const = 0;

        virtual void register_transport_callbacks(webrtc::P2PTransportChannel* transport_channel) = 0;

        virtual void state_updated(bool is_connected) = 0;

        virtual void start() = 0;

        virtual void rtp_packet_received(const webrtc::RtpPacketReceived& packet) = 0;

        void init_connection(bool supports_packet_sending = false);

        void handle_role_conflict();

        void add_incoming_smart_source(const std::string& endpoint, const models::MediaContent& media_content, bool force = false);

        void remove_incoming_audio(const std::string& endpoint);

    public:
        void close() override;

        std::unique_ptr<media::tracks::MediaTrackInterface> add_outgoing_track(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) override;

        void add_incoming_audio_track(const std::weak_ptr<media::RemoteAudioSink>& sink) override;

        void add_incoming_video_track(const std::weak_ptr<media::RemoteVideoSink>& sink, bool is_screen_cast) override;

        models::PeerIceParameters local_ice_parameters();

        std::unique_ptr<webrtc::SSLFingerprint> local_fingerprint() const;

        void send_data_channel_message(const bytes::binary& data) const override;

        static webrtc::CryptoOptions get_default_crypto_options();

        std::vector<std::string> get_endpoints() const;

        ConnectionMode get_connection_mode() const override;

        void enable_audio_incoming(bool enable) override;

        void enable_video_incoming(bool enable, bool is_screen_cast) override;
    };

} // wrtc::interfaces
