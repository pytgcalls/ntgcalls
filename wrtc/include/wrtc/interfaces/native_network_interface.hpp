//
// Created by Laky64 on 01/10/24.
//

#pragma once

#include <pc/dtls_transport.h>
#include <api/scoped_refptr.h>
#include <pc/dtls_srtp_transport.h>
#include <p2p/base/dtls_transport.h>
#include <rtc_base/rtc_certificate.h>
#include <p2p/base/p2p_transport_channel.h>
#include <p2p/client/basic_port_allocator.h>
#include <wrtc/models/peer_ice_parameters.hpp>
#include <wrtc/interfaces/network_interface.hpp>
#include <wrtc/interfaces/media/channel_manager.hpp>
#include <wrtc/interfaces/sctp_data_channel_provider_interface_impl.hpp>
#include <wrtc/interfaces/media/channels/outgoing_audio_channel.hpp>
#include <wrtc/interfaces/media/channels/outgoing_video_channel.hpp>
#include <wrtc/interfaces/media/channels/incoming_audio_channel.hpp>
#include <wrtc/interfaces/media/channels/incoming_video_channel.hpp>

namespace wrtc {

    class NativeNetworkInterface: public sigslot::has_slots<>, public NetworkInterface, public std::enable_shared_from_this<NativeNetworkInterface> {
        struct H264FormatParameters {
            std::string profileLevelId;
            std::string packetizationMode;
            std::string levelAssymetryAllowed;
        };

        std::unique_ptr<cricket::BasicPortAllocator> portAllocator;
        rtc::scoped_refptr<rtc::RTCCertificate> localCertificate;
        std::unique_ptr<webrtc::AsyncDnsResolverFactoryInterface> asyncResolverFactory;

        void DtlsReadyToSend(bool isReadyToSend);

        void resetDtlsSrtpTransport();

        void OnTransportWritableState_n(rtc::PacketTransportInternal*);

        void OnTransportReceivingState_n(rtc::PacketTransportInternal*);

        void UpdateAggregateStates_n();

        void transportStateChanged(cricket::IceTransportInternal *transport);

        static std::vector<webrtc::SdpVideoFormat> filterSupportedVideoFormats(std::vector<webrtc::SdpVideoFormat> const &formats);

        static H264FormatParameters parseH264FormatParameters(webrtc::SdpVideoFormat const &format);

        static int getH264ProfileLevelIdPriority(std::string const &profileLevelId);

        static int getH264PacketizationModePriority(std::string const &packetizationMode);

        static int getH264LevelAssymetryAllowedPriority(std::string const &levelAssymetryAllowed);

    protected:
        std::mutex mutex;
        std::unique_ptr<webrtc::Call> call;
        webrtc::LocalAudioSinkAdapter audioSink;
        LocalVideoAdapter videoSink;
        std::weak_ptr<RemoteAudioSink> remoteAudioSink;
        std::weak_ptr<RemoteVideoSink> remoteVideoSink;
        std::weak_ptr<RemoteVideoSink> remoteScreenCastSink;
        std::unique_ptr<ChannelManager> channelManager;
        std::unique_ptr<OutgoingAudioChannel> audioChannel;
        std::unique_ptr<OutgoingVideoChannel> videoChannel;
        PeerIceParameters localParameters, remoteParameters;
        std::unique_ptr<cricket::DtlsTransport> dtlsTransport;
        std::unique_ptr<webrtc::DtlsSrtpTransport> dtlsSrtpTransport;
        std::unique_ptr<cricket::P2PTransportChannel> transportChannel;
        std::vector<webrtc::SdpVideoFormat> availableVideoFormats;
        std::unique_ptr<SctpDataChannelProviderInterfaceImpl> dataChannelInterface;
        std::map<std::string, std::unique_ptr<IncomingAudioChannel>> incomingAudioChannels;
        std::map<std::string, std::unique_ptr<IncomingVideoChannel>> incomingVideoChannels;
        std::map<std::string, MediaContent> pendingContent;
        bool connected = false, failed = false;

        virtual std::pair<cricket::ServerAddresses, std::vector<cricket::RelayServerConfig>> getStunAndTurnServers() = 0;

        virtual cricket::RelayPortFactoryInterface* getRelayPortFactory() = 0;

        virtual void setPortAllocatorFlags(cricket::BasicPortAllocator* portAllocator) = 0;

        virtual int getRegatherOnFailedNetworksInterval() = 0;

        virtual bool getCustomParameterBool(const std::string& name) const = 0;

        virtual bool supportsRenomination() const = 0;

        virtual cricket::IceRole iceRole() const = 0;

        virtual cricket::IceMode iceMode() const = 0;

        virtual int candidatePoolSize() const = 0;

        virtual bool isGroupConnection() const = 0;

        virtual std::optional<rtc::SSLRole> dtlsRole() const = 0;

        virtual void registerTransportCallbacks(cricket::P2PTransportChannel* transportChannel) = 0;

        virtual void stateUpdated(bool isConnected) = 0;

        virtual void start() = 0;

        virtual void RtpPacketReceived(const webrtc::RtpPacketReceived& packet) = 0;

        void close() override;

        std::unique_ptr<MediaTrackInterface> addOutgoingTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) override;

        void addIncomingAudioTrack(const std::weak_ptr<RemoteAudioSink>& sink) override;

        void addIncomingVideoTrack(const std::weak_ptr<RemoteVideoSink>& sink, bool isScreenCast) override;

        void initConnection(bool supportsPacketSending = false);

        void addIncomingSmartSource(const std::string& endpoint, const MediaContent& mediaContent, bool force = false);

        void removeIncomingAudio(const std::string& endpoint);

    public:
        PeerIceParameters localIceParameters();

        std::unique_ptr<rtc::SSLFingerprint> localFingerprint() const;

        void sendDataChannelMessage(const bytes::binary& data) const override;

        static webrtc::CryptoOptions getDefaultCryptoOptions();

        std::vector<std::string> getEndpoints() const;

        void enableAudioIncoming(bool enable) override;

        void enableVideoIncoming(bool enable, bool isScreenCast) override;
    };

} // wrtc
