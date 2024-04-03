//
// Created by Laky64 on 29/03/2024.
//

#pragma once
#include <p2p/base/p2p_transport_channel.h>
#include <p2p/client/basic_port_allocator.h>
#include <p2p/client/relay_port_factory_interface.h>
#include <rtc_base/rtc_certificate.h>
#include <pc/dtls_srtp_transport.h>
#include <pc/dtls_transport.h>
#include <memory>

#include "instance_networking.hpp"
#include "network_interface.hpp"
#include "sctp_data_channel_provider_interface_impl.hpp"
#include "media/channel_manager.hpp"
#include "media/channels/outgoing_audio_channel.hpp"
#include "media/channels/outgoing_video_channel.hpp"
#include "wrtc/models/content_negotiation_context.hpp"
#include "wrtc/models/peer_ice_parameters.hpp"
#include "wrtc/models/rtc_server.hpp"


namespace wrtc {
    class NativeConnection final : public sigslot::has_slots<>, public InstanceNetworking, public NetworkInterface {
        bool connected = false, failed = false;
        bool isOutgoing, enableTCP, enableP2P;
        int64_t lastDisconnectedTimestamp = 0;
        std::vector<RTCServer> rtcServers;
        PeerIceParameters localParameters, remoteParameters;
        rtc::scoped_refptr<rtc::RTCCertificate> localCertificate;
        std::unique_ptr<cricket::DtlsTransport> dtlsTransport;
        std::unique_ptr<webrtc::DtlsSrtpTransport> dtlsSrtpTransport;
        std::unique_ptr<cricket::RelayPortFactoryInterface> relayPortFactory;
        std::unique_ptr<cricket::BasicPortAllocator> portAllocator;
        std::unique_ptr<webrtc::AsyncDnsResolverFactoryInterface> asyncResolverFactory;
        std::unique_ptr<cricket::P2PTransportChannel> transportChannel;
        std::unique_ptr<SctpDataChannelProviderInterfaceImpl> dataChannelInterface;
        absl::optional<RouteDescription> currentRouteDescription;
        absl::optional<ConnectionDescription> currentConnectionDescription;
        std::unique_ptr<webrtc::RtcEventLogNull> eventLog;
        std::unique_ptr<webrtc::TaskQueueFactory> taskQueueFactory;
        std::unique_ptr<ContentNegotiationContext> contentNegotiationContext;
        std::optional<std::string> audioChannelId, videoChannelId;
        std::unique_ptr<webrtc::Call> call;
        std::unique_ptr<ChannelManager> channelManager;
        std::unique_ptr<OutgoingAudioChannel> audioChannel;
        std::unique_ptr<OutgoingVideoChannel> videoChannel;
        webrtc::LocalAudioSinkAdapter audioSink;
        std::optional<webrtc::VideoTrackSourceInterface*> videoSink;

        void notifyStateUpdated() const;

        void DtlsReadyToSend(bool isReadyToSend);

        void candidateGathered(cricket::IceTransportInternal *transport, const cricket::Candidate &candidate);

        void transportStateChanged(cricket::IceTransportInternal *transport);

        void transportRouteChanged(absl::optional<rtc::NetworkRoute> route);

        void UpdateAggregateStates_n();

        void OnTransportWritableState_n(rtc::PacketTransportInternal *transport);

        void OnTransportReceivingState_n(rtc::PacketTransportInternal *transport);

        void candidatePairChanged(cricket::CandidatePairChangeEvent const &event);

        void checkConnectionTimeout();

        void start();

        void resetDtlsSrtpTransport();

        void createChannels();
    public:
        explicit NativeConnection(std::vector<RTCServer> rtcServers, bool enableTCP, bool enableP2P, bool isOutgoing);

        ~NativeConnection() override;

        static webrtc::CryptoOptions getDefaultCryptoOptions();

        void close() override;

        void sendDataChannelMessage(const bytes::binary& data) const override;

        void addIceCandidate(const IceCandidate& rawCandidate) const override;

        void setRemoteParams(PeerIceParameters remoteIceParameters, std::unique_ptr<rtc::SSLFingerprint> fingerprint, const std::string& sslSetup);

        std::unique_ptr<ContentNegotiationContext::NegotiationContents> getPendingOffer() const;

        std::unique_ptr<ContentNegotiationContext::NegotiationContents> setPendingAnwer(std::unique_ptr<ContentNegotiationContext::NegotiationContents> answer);

        void addTrack(MediaStreamTrack* mediaStreamTrack) override;

        std::unique_ptr<rtc::SSLFingerprint> localFingerprint() const;

        PeerIceParameters localIceParameters();
    };

} // wrtc
