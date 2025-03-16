//
// Created by Laky64 on 30/03/2024.
//

#pragma once
#include <p2p/base/transport_description_factory.h>
#include <rtc_base/unique_id_generator.h>
#include <pc/media_session.h>
#include <wrtc/models/media_content.hpp>

namespace wrtc {

    class ContentNegotiationContext {
    public:
        struct OutgoingChannel {
            std::string id;
            MediaContent content;

            OutgoingChannel(std::string id, MediaContent content):id(std::move(id)), content(std::move(content)) {}
        };

        struct PendingOutgoingChannel {
            cricket::MediaDescriptionOptions description;
            uint32_t ssrc = 0;
            std::vector<SsrcGroup> ssrcGroups;
            explicit PendingOutgoingChannel(cricket::MediaDescriptionOptions &&description):description(std::move(description)) {}
        };

        struct NegotiationContents {
            uint32_t exchangeId = 0;
            std::vector<MediaContent> contents;
        };

        struct PendingOutgoingOffer {
            uint32_t exchangeId = 0;
        };


        struct CoordinatedState {
            std::vector<MediaContent> outgoingContents;
            std::vector<MediaContent> incomingContents;
        };

    private:
        bool isOutgoing, needNegotiation;
        rtc::UniqueRandomIdGenerator* uniqueRandomIdGenerator;
        std::unique_ptr<cricket::TransportDescriptionFactory> transportDescriptionFactory;
        std::unique_ptr<cricket::MediaSessionDescriptionFactory> sessionDescriptionFactory;
        std::vector<webrtc::RtpHeaderExtensionCapability> rtpAudioExtensions;
        std::vector<webrtc::RtpHeaderExtensionCapability> rtpVideoExtensions;
        std::vector<PendingOutgoingChannel> outgoingChannelDescriptions;
        std::unique_ptr<PendingOutgoingOffer> pendingOutgoingOffer;
        std::vector<std::string> channelIdOrder;
        std::vector<MediaContent> incomingChannels;
        std::vector<OutgoingChannel> outgoingChannels;
        int nextOutgoingChannelId = 0;

        [[nodiscard]] std::unique_ptr<cricket::SessionDescription> currentSessionDescriptionFromCoordinatedState() const;

        static cricket::ContentInfo convertSignalingContentToContentInfo(const std::string &contentId, const MediaContent &content, webrtc::RtpTransceiverDirection direction);

        static MediaContent convertContentInfoToSignalingContent(const cricket::ContentInfo &content);

        static cricket::ContentInfo createInactiveContentInfo(const std::string &contentId);

        static cricket::MediaDescriptionOptions getIncomingContentDescription(const MediaContent &content);

        void setAnswer(std::unique_ptr<NegotiationContents> &&answer);

        std::unique_ptr<NegotiationContents> getAnswer(std::unique_ptr<NegotiationContents> &&offer);

    public:
        ContentNegotiationContext(
            const webrtc::FieldTrialsView& fieldTrials,
            bool isOutgoing,
            cricket::MediaEngineInterface *mediaEngine,
            rtc::UniqueRandomIdGenerator *uniqueRandomIdGenerator,
            webrtc::PayloadTypeSuggester *payloadTypeSuggester
        );

        void copyCodecsFromChannelManager(cricket::MediaEngineInterface *mediaEngine, bool randomize);

        std::string addOutgoingChannel(const webrtc::MediaStreamTrackInterface* track);

        std::unique_ptr<NegotiationContents> getPendingOffer();

        std::unique_ptr<NegotiationContents> setPendingAnswer(std::unique_ptr<NegotiationContents>&& answer);

        [[nodiscard]] std::unique_ptr<CoordinatedState> coordinatedState() const;

        [[nodiscard]] std::optional<uint32_t> outgoingChannelSsrc(const std::string &id) const;
    };

} // wrtc