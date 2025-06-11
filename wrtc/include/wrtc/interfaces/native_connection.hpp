//
// Created by Laky64 on 29/03/2024.
//

#pragma once
#include <p2p/client/relay_port_factory_interface.h>

#include <wrtc/interfaces/network_interface.hpp>
#include <wrtc/interfaces/native_network_interface.hpp>
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
        bool isOutgoing, enableP2P;
        int64_t lastDisconnectedTimestamp = 0;
        std::vector<RTCServer> rtcServers;
        std::unique_ptr<webrtc::RelayPortFactoryInterface> relayPortFactory;
        std::optional<RouteDescription> currentRouteDescription;
        std::optional<ConnectionDescription> currentConnectionDescription;
        std::unique_ptr<webrtc::RtcEventLogNull> eventLog;
        std::unique_ptr<ContentNegotiationContext> contentNegotiationContext;
        std::optional<std::string> audioChannelId, videoChannelId;

        void notifyStateUpdated();

        void candidateGathered(webrtc::IceTransportInternal *transport, const webrtc::Candidate &candidate);

        void transportRouteChanged(std::optional<webrtc::NetworkRoute> route);

        void candidatePairChanged(webrtc::CandidatePairChangeEvent const &event);

        void checkConnectionTimeout();

        void start() override;

        bool getCustomParameterBool(const std::string& name) const override;

        static CandidateDescription connectionDescriptionFromCandidate(const webrtc::Candidate &candidate);

        webrtc::RelayPortFactoryInterface* getRelayPortFactory() override;

        std::pair<webrtc::ServerAddresses, std::vector<webrtc::RelayServerConfig>> getStunAndTurnServers() override;

        void setPortAllocatorFlags(webrtc::BasicPortAllocator* portAllocator) override;

        int getRegatherOnFailedNetworksInterval() override;

        webrtc::IceRole iceRole() const override;

        webrtc::IceMode iceMode() const override;

        void registerTransportCallbacks(webrtc::P2PTransportChannel* transportChannel) override;

        std::optional<webrtc::SSLRole> dtlsRole() const override;

        bool supportsRenomination() const override;

        void stateUpdated(bool isConnected) override;

        int candidatePoolSize() const override;

        void RtpPacketReceived(const webrtc::RtpPacketReceived& packet) override {}

        bool isGroupConnection() const override;
    public:
        explicit NativeConnection(std::vector<RTCServer> rtcServers, bool enableP2P, bool isOutgoing);

        void open() override;

        void close() override;

        void createChannels();

        void addIceCandidate(const IceCandidate& rawCandidate) const override;

        void setRemoteParams(PeerIceParameters remoteIceParameters, std::unique_ptr<webrtc::SSLFingerprint> fingerprint, const std::string& sslSetup);

        std::unique_ptr<ContentNegotiationContext::NegotiationContents> getPendingOffer() const;

        std::unique_ptr<ContentNegotiationContext::NegotiationContents> setPendingAnswer(std::unique_ptr<ContentNegotiationContext::NegotiationContents> answer) const;

        std::unique_ptr<MediaTrackInterface> addOutgoingTrack(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) override;
    };
} // wrtc