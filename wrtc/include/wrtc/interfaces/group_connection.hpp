//
// Created by Laky64 on 01/10/24.
//

#pragma once

#include <nlohmann/json.hpp>
#include <utility>
#include <wrtc/interfaces/network_interface.hpp>
#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/models/response_payload.hpp>

namespace wrtc {
    using nlohmann::json;

    class GroupConnection final: public NativeNetworkInterface {
    public:
        enum class Mode {
            None,
            Rtc,
            Rtmp
        };

        explicit GroupConnection(bool isPresentation);

        std::string getJoinPayload();

        void addIceCandidate(const IceCandidate& rawCandidate) const override;

        void setRemoteParams(PeerIceParameters remoteIceParameters, std::unique_ptr<rtc::SSLFingerprint> fingerprint);

        void setConnectionMode(Mode mode);

        void createChannels(const ResponsePayload::Media& media);

        uint32_t addIncomingVideo(const std::string& endpoint, const std::vector<SsrcGroup>& ssrcGroups);

        bool removeIncomingVideo(const std::string& endpoint);

        void open() override;

        void close() override;

        ResponsePayload::Media getMediaConfig() const;

    private:
        int64_t lastNetworkActivityMs = 0;
        uint32_t outgoingAudioSsrc = 0, outgoingVideoSsrc = 0;
        std::vector<SsrcGroup> outgoingVideoSsrcGroups;
        bool isPresentation = false;
        bool isRtcConnected = false, isRtmpConnected = false;
        bool lastEffectivelyConnected = false;
        Mode connectionMode = Mode::None;
        ResponsePayload::Media mediaConfig;

        bool supportsRenomination() const override;

        cricket::IceRole iceRole() const override;

        cricket::IceMode iceMode() const override;

        std::optional<rtc::SSLRole> dtlsRole() const override;

        std::pair<cricket::ServerAddresses, std::vector<cricket::RelayServerConfig>> getStunAndTurnServers() override;

        cricket::RelayPortFactoryInterface* getRelayPortFactory() override;

        void registerTransportCallbacks(cricket::P2PTransportChannel* transportChannel) override;

        int getRegatherOnFailedNetworksInterval() override;

        bool getCustomParameterBool(const std::string& name) const override;

        void setPortAllocatorFlags(cricket::BasicPortAllocator* portAllocator) override;

        void start() override;

        void restartDataChannel();

        void generateSsrcs();

        void stateUpdated(bool isConnected) override;

        int candidatePoolSize() const override;

        void updateIsConnected();

        void RtpPacketReceived(const webrtc::RtpPacketReceived& packet) override;

        void addIncomingAudio(uint32_t ssrc, const std::string& endpoint);

        void beginAudioChannelCleanupTimer();

        bool isGroupConnection() const override;
    };

} // wrtc
