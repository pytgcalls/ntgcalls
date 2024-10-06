//
// Created by Laky64 on 29/03/2024.
//

#pragma once
#include <p2p/client/relay_port_factory_interface.h>

#include <wrtc/interfaces/network_interface.hpp>
#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/interfaces/media/local_video_adapter.hpp>
#include <wrtc/interfaces/media/channels/outgoing_audio_channel.hpp>
#include <wrtc/models/content_negotiation_context.hpp>
#include <wrtc/models/rtc_server.hpp>
#include <nlohmann/json.hpp>

#include <wrtc/models/connection_description.hpp>
#include <wrtc/models/route_description.hpp>

namespace wrtc {
    using nlohmann::json;

    class NativeConnection final : public NativeNetworkInterface {
        json customParameters;
        std::atomic_bool isExiting;
        bool isOutgoing, enableP2P;
        int64_t lastDisconnectedTimestamp = 0;
        std::vector<RTCServer> rtcServers;
        std::unique_ptr<cricket::RelayPortFactoryInterface> relayPortFactory;
        absl::optional<RouteDescription> currentRouteDescription;
        absl::optional<ConnectionDescription> currentConnectionDescription;
        std::unique_ptr<webrtc::RtcEventLogNull> eventLog;
        std::unique_ptr<ContentNegotiationContext> contentNegotiationContext;
        std::optional<std::string> audioChannelId, videoChannelId;

        void notifyStateUpdated() const;

        void candidateGathered(cricket::IceTransportInternal *transport, const cricket::Candidate &candidate);

        void transportRouteChanged(absl::optional<rtc::NetworkRoute> route);

        void candidatePairChanged(cricket::CandidatePairChangeEvent const &event);

        void checkConnectionTimeout();

        void start() override;

        bool getCustomParameterBool(const std::string& name) const override;

        static CandidateDescription connectionDescriptionFromCandidate(const cricket::Candidate &candidate);

        cricket::RelayPortFactoryInterface* getRelayPortFactory() override;

        std::pair<cricket::ServerAddresses, std::vector<cricket::RelayServerConfig>> getStunAndTurnServers() override;

        void setPortAllocatorFlags(cricket::BasicPortAllocator* portAllocator) override;

        int getRegatherOnFailedNetworksInterval() override;

        cricket::IceRole iceRole() const override;

        cricket::IceMode iceMode() const override;

        void registerTransportCallbacks(cricket::P2PTransportChannel* transportChannel) override;

        std::optional<rtc::SSLRole> dtlsRole() const override;

        bool supportsPacketSending() const override;

        bool supportsRenomination() const override;

        void stateUpdated(bool isConnected) override;

        int candidatePoolSize() const override;

        void RtpPacketReceived(const webrtc::RtpPacketReceived& packet) override {}
    public:
        explicit NativeConnection(std::vector<RTCServer> rtcServers, bool enableP2P, bool isOutgoing);

        ~NativeConnection() override;

        void close() override;

        void createChannels();

        void addIceCandidate(const IceCandidate& rawCandidate) const override;

        void setRemoteParams(PeerIceParameters remoteIceParameters, std::unique_ptr<rtc::SSLFingerprint> fingerprint, const std::string& sslSetup);

        std::unique_ptr<ContentNegotiationContext::NegotiationContents> getPendingOffer() const;

        std::unique_ptr<ContentNegotiationContext::NegotiationContents> setPendingAnswer(std::unique_ptr<ContentNegotiationContext::NegotiationContents> answer) const;

        std::unique_ptr<MediaTrackInterface> addTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) override;
    };

} // wrtc