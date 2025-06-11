//
// Created by Laky64 on 30/03/2024.
//

#include <wrtc/models/content_negotiation_context.hpp>

#include <media/base/media_engine.h>
#include <p2p/base/transport_description.h>
#include <rtc_base/rtc_certificate_generator.h>
#include <pc/webrtc_session_description_factory.h>

#include <wrtc/exceptions.hpp>

namespace wrtc {
    ContentNegotiationContext::ContentNegotiationContext(
        const webrtc::FieldTrialsView& fieldTrials,
        const bool isOutgoing,
        webrtc::MediaEngineInterface *mediaEngine,
        webrtc::UniqueRandomIdGenerator *uniqueRandomIdGenerator,
        webrtc::PayloadTypeSuggester *payloadTypeSuggester
    ) :isOutgoing(isOutgoing), uniqueRandomIdGenerator(uniqueRandomIdGenerator) {
        transportDescriptionFactory = std::make_unique<webrtc::TransportDescriptionFactory>(fieldTrials);
        const auto tempCertificate = webrtc::RTCCertificateGenerator::GenerateCertificate(
            webrtc::KeyParams(webrtc::KT_ECDSA),
            std::nullopt
        );
        transportDescriptionFactory->set_certificate(tempCertificate);
        codecLookupHelper = std::make_unique<CodecLookupHelper>(
            mediaEngine,
            transportDescriptionFactory.get(),
            payloadTypeSuggester
        );
        sessionDescriptionFactory = std::make_unique<webrtc::MediaSessionDescriptionFactory>(
            mediaEngine,
            true,
            uniqueRandomIdGenerator,
            transportDescriptionFactory.get(),
            codecLookupHelper.get()
        );
        needNegotiation = true;
    }

    ContentNegotiationContext::~ContentNegotiationContext() {
        sessionDescriptionFactory = nullptr;
        transportDescriptionFactory = nullptr;
        codecLookupHelper = nullptr;
        uniqueRandomIdGenerator = nullptr;
        outgoingChannelDescriptions.clear();
        channelIdOrder.clear();
        incomingChannels.clear();
        outgoingChannels.clear();
        rtpAudioExtensions.clear();
        rtpVideoExtensions.clear();
        pendingOutgoingOffer.reset();
    }

    void ContentNegotiationContext::copyCodecsFromChannelManager(webrtc::MediaEngineInterface *mediaEngine, const bool randomize) {
        int absSendTimeUriId = 2;
        int transportSequenceNumberUriId = 3;
        int videoRotationUri = 13;

        if (randomize) {
            absSendTimeUriId = 3;
            transportSequenceNumberUriId = 2;
            videoRotationUri = 4;
        }
        rtpAudioExtensions.emplace_back(webrtc::RtpExtension::kAbsSendTimeUri, absSendTimeUriId);
        rtpAudioExtensions.emplace_back(webrtc::RtpExtension::kTransportSequenceNumberUri, transportSequenceNumberUriId);
        rtpVideoExtensions.emplace_back(webrtc::RtpExtension::kAbsSendTimeUri, absSendTimeUriId);
        rtpVideoExtensions.emplace_back(webrtc::RtpExtension::kTransportSequenceNumberUri, transportSequenceNumberUriId);
        rtpVideoExtensions.emplace_back(webrtc::RtpExtension::kVideoRotationUri, videoRotationUri);
    }

    std::string ContentNegotiationContext::addOutgoingChannel(const webrtc::MediaStreamTrackInterface* track) {
        std::string channelId = track->id();
        webrtc::MediaType mappedMediaType;
        std::vector<webrtc::RtpHeaderExtensionCapability> rtpExtensions;
        if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
            mappedMediaType = webrtc::MediaType::AUDIO;
            rtpExtensions = rtpAudioExtensions;
        } else {
            mappedMediaType = webrtc::MediaType::VIDEO;
            rtpExtensions = rtpVideoExtensions;
        }
        webrtc::MediaDescriptionOptions offerDescription(mappedMediaType, channelId, webrtc::RtpTransceiverDirection::kSendOnly, false);
        offerDescription.header_extensions = rtpExtensions;
        if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
            offerDescription.AddAudioSender(channelId, {channelId});
        } else {
            const webrtc::SimulcastLayerList simulcastLayers;
            offerDescription.AddVideoSender(channelId, {channelId}, {}, simulcastLayers, 1);
        }
        outgoingChannelDescriptions.emplace_back(std::move(offerDescription));
        needNegotiation = true;
        return channelId;
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> ContentNegotiationContext::getPendingOffer() {
        if (!needNegotiation) {
            return nullptr;
        }
        if (pendingOutgoingOffer) {
            return nullptr;
        }
        needNegotiation = false;
        pendingOutgoingOffer = std::make_unique<PendingOutgoingOffer>();
        pendingOutgoingOffer->exchangeId = uniqueRandomIdGenerator->GenerateId();
        auto currentSessionDescription = currentSessionDescriptionFromCoordinatedState();
        webrtc::MediaSessionOptions offerOptions;
        offerOptions.offer_extmap_allow_mixed = true;
        offerOptions.bundle_enabled = true;
        for (const auto &id : channelIdOrder) {
            bool found = false;
            for (const auto &channel : outgoingChannelDescriptions) {
                if (channel.description.mid == id) {
                    found = true;
                    offerOptions.media_description_options.push_back(channel.description);

                    break;
                }
            }
            for (const auto &content : incomingChannels) {
                if (std::to_string(content.ssrc) == id) {
                    found = true;
                    offerOptions.media_description_options.push_back(getIncomingContentDescription(content));
                    break;
                }
            }
            if (!found) {
                webrtc::MediaDescriptionOptions contentDescription(webrtc::MediaType::AUDIO, "_" + id, webrtc::RtpTransceiverDirection::kInactive, false);
                offerOptions.media_description_options.push_back(contentDescription);
            }
        }
        for (const auto &channel : outgoingChannelDescriptions) {
            if (std::ranges::find(channelIdOrder, channel.description.mid) == channelIdOrder.end()) {
                channelIdOrder.push_back(channel.description.mid);
                offerOptions.media_description_options.push_back(channel.description);
            }
            for (const auto &content : incomingChannels) {
                if (std::ranges::find(channelIdOrder, std::to_string(content.ssrc)) == channelIdOrder.end()) {
                    channelIdOrder.push_back(std::to_string(content.ssrc));
                    offerOptions.media_description_options.push_back(getIncomingContentDescription(content));
                }
            }
        }
        auto offerOrError = sessionDescriptionFactory->CreateOfferOrError(
            offerOptions, currentSessionDescription.get()
        );
        if (!offerOrError.ok()) {
            RTC_LOG(LS_ERROR) << "Failed to create offer: " << offerOrError.error().message();
            wrapRTCError(offerOrError.error());
        }
        auto offer = offerOrError.MoveValue();
        auto mappedOffer = std::make_unique<NegotiationContents>();
        mappedOffer->exchangeId = pendingOutgoingOffer->exchangeId;
        for (const auto &content : offer->contents()) {
            auto mappedContent = convertContentInfoToSignalingContent(content);
            if (content.media_description()->direction() == webrtc::RtpTransceiverDirection::kSendOnly) {
                mappedOffer->contents.push_back(std::move(mappedContent));
                for (auto &channel : outgoingChannelDescriptions) {
                    if (channel.description.mid == content.mid()) {
                        channel.ssrc = mappedContent.ssrc;
                        channel.ssrcGroups = mappedContent.ssrcGroups;
                    }
                }
            }
        }
        return mappedOffer;
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> ContentNegotiationContext::setPendingAnswer(std::unique_ptr<NegotiationContents> answer) {
        if (!answer) {
            return nullptr;
        }
        if (pendingOutgoingOffer) {
            if (answer->exchangeId == pendingOutgoingOffer->exchangeId) {
                setAnswer(std::move(answer));
                return nullptr;
            }
            if (!isOutgoing) {
                pendingOutgoingOffer.reset();
                return getAnswer(std::move(answer));
            }
            return nullptr;
        }
        return getAnswer(std::move(answer));
    }

    std::unique_ptr<ContentNegotiationContext::CoordinatedState> ContentNegotiationContext::coordinatedState() const {
        auto result = std::make_unique<CoordinatedState>();
        result->incomingContents = incomingChannels;
        for (const auto &channel : outgoingChannels) {
            bool found = false;
            for (const auto &channelDescription : outgoingChannelDescriptions) {
                if (channelDescription.description.mid == channel.id) {
                    found = true;
                    break;
                }
            }
            if (found) {
                result->outgoingContents.push_back(channel.content);
            }
        }
        return result;
    }

    std::optional<uint32_t> ContentNegotiationContext::outgoingChannelSsrc(const std::string &id) const {
        for (const auto &channel : outgoingChannels) {
            bool found = false;
            for (const auto &channelDescription : outgoingChannelDescriptions) {
                if (channelDescription.description.mid == channel.id) {
                    found = true;
                    break;
                }
            }
            if (found && channel.id == id) {
                if (channel.content.ssrc != 0) {
                    return channel.content.ssrc;
                }
            }
        }
        return std::nullopt;
    }

    std::unique_ptr<webrtc::SessionDescription> ContentNegotiationContext::currentSessionDescriptionFromCoordinatedState() const {
        if (channelIdOrder.empty()) {
            return nullptr;
        }
        auto sessionDescription = std::make_unique<webrtc::SessionDescription>();
        for (const auto &id : channelIdOrder) {
            bool found = false;
            for (const auto &channel : incomingChannels) {
                if (std::to_string(channel.ssrc) == id) {
                    found = true;
                    auto mappedContent = convertSignalingContentToContentInfo(std::to_string(channel.ssrc), channel, webrtc::RtpTransceiverDirection::kRecvOnly);
                    auto localCertificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                    if (localCertificate) {
                        fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                    }
                    std::vector<std::string> transportOptions;
                    webrtc::TransportDescription transportDescription(
                        transportOptions,
                        "ufrag",
                        "pwd",
                        webrtc::IceMode::ICEMODE_FULL,
                        webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    webrtc::TransportInfo transportInfo(std::to_string(channel.ssrc), transportDescription);
                    sessionDescription->AddTransportInfo(transportInfo);
                    sessionDescription->AddContent(std::move(mappedContent));
                    break;
                }
            }
            for (const auto &channel : outgoingChannels) {
                if (channel.id == id) {
                    found = true;
                    auto mappedContent = convertSignalingContentToContentInfo(channel.id, channel.content, webrtc::RtpTransceiverDirection::kSendOnly);
                    auto localCertificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                    if (localCertificate) {
                        fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                    }
                    std::vector<std::string> transportOptions;
                    webrtc::TransportDescription transportDescription(
                        transportOptions,
                        "ufrag",
                        "pwd",
                        webrtc::IceMode::ICEMODE_FULL,
                        webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    webrtc::TransportInfo transportInfo(mappedContent.mid(), transportDescription);
                    sessionDescription->AddTransportInfo(transportInfo);
                    sessionDescription->AddContent(std::move(mappedContent));
                    break;
                }
            }

            if (!found) {
                auto mappedContent = createInactiveContentInfo("_" + id);
                auto localCertificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                if (localCertificate) {
                    fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                }
                std::vector<std::string> transportOptions;
                webrtc::TransportDescription transportDescription(
                    transportOptions,
                    "ufrag",
                    "pwd",
                    webrtc::IceMode::ICEMODE_FULL,
                    webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                    fingerprint.get()
                );
                webrtc::TransportInfo transportInfo(mappedContent.mid(), transportDescription);
                sessionDescription->AddTransportInfo(transportInfo);
                sessionDescription->AddContent(std::move(mappedContent));
            }
        }
        return sessionDescription;
    }

    webrtc::ContentInfo ContentNegotiationContext::convertSignalingContentToContentInfo(const std::string &contentId, const MediaContent &content, webrtc::RtpTransceiverDirection direction) {
        std::unique_ptr<webrtc::MediaContentDescription> contentDescription;
        switch (content.type) {
            case MediaContent::Type::Audio: {
                auto audioDescription = std::make_unique<webrtc::AudioContentDescription>();
                for (const auto &[id, name, clockrate, channels, feedbackTypes, parameters] : content.payloadTypes) {
                    auto mappedCodec = webrtc::CreateAudioCodec(
                        static_cast<int>(id),
                        name,
                        static_cast<int>(clockrate),
                        channels
                    );
                    for (const auto &parameter : parameters) {
                        mappedCodec.params.insert(parameter);
                    }
                    for (const auto &[type, subtype] : feedbackTypes) {
                        mappedCodec.AddFeedbackParam(webrtc::FeedbackParam(type, subtype));
                    }
                    audioDescription->AddCodec(mappedCodec);
                }
                contentDescription = std::move(audioDescription);
                break;
            }
            case MediaContent::Type::Video: {
                auto videoDescription = std::make_unique<webrtc::VideoContentDescription>();
                for (const auto &[id, name, clockrate, channels, feedbackTypes, parameters] : content.payloadTypes) {
                    webrtc::SdpVideoFormat videoFormat(name);
                    for (const auto &parameter : parameters) {
                        videoFormat.parameters.insert(parameter);
                    }
                    webrtc::Codec mappedCodec = webrtc::CreateVideoCodec(videoFormat);
                    mappedCodec.id = static_cast<int>(id);
                    for (const auto &parameter : parameters) {
                        mappedCodec.params.insert(parameter);
                    }
                    for (const auto &[type, subtype] : feedbackTypes) {
                        mappedCodec.AddFeedbackParam(webrtc::FeedbackParam(type, subtype));
                    }
                    videoDescription->AddCodec(mappedCodec);
                }
                contentDescription = std::move(videoDescription);
                break;
            }
            default: {
                throw RTCException("Unknown media type");
            }
        }
        webrtc::StreamParams streamParams;
        streamParams.id = contentId;
        streamParams.set_stream_ids({ contentId });
        streamParams.add_ssrc(content.ssrc);
        for (const auto &[semantics, ssrcs] : content.ssrcGroups) {
            streamParams.ssrc_groups.emplace_back(semantics, ssrcs);
            for (const auto &ssrc : ssrcs) {
                if (!streamParams.has_ssrc(ssrc)) {
                    streamParams.add_ssrc(ssrc);
                }
            }
        }
        contentDescription->AddStream(streamParams);
        for (const auto &extension : content.rtpExtensions) {
            contentDescription->AddRtpHeaderExtension(extension);
        }
        contentDescription->set_direction(direction);
        contentDescription->set_rtcp_mux(true);
        return {
            webrtc::MediaProtocolType::kRtp,
            contentId,
            std::move(contentDescription),
        };
    }

    MediaContent ContentNegotiationContext::convertContentInfoToSignalingContent(const webrtc::ContentInfo &content) {
        MediaContent mappedContent;
        switch (content.media_description()->type()) {
            case webrtc::MediaType::AUDIO:
                mappedContent.type = MediaContent::Type::Audio;
                for (const auto &codec : content.media_description()->as_audio()->codecs()) {
                    PayloadType mappedPayloadType;
                    mappedPayloadType.id = codec.id;
                    mappedPayloadType.name = codec.name;
                    mappedPayloadType.clockrate = codec.clockrate;
                    mappedPayloadType.channels = static_cast<uint32_t>(codec.channels);
                    for (const auto &feedbackType : codec.feedback_params.params()) {
                        FeedbackType mappedFeedbackType;
                        mappedFeedbackType.type = feedbackType.id();
                        mappedFeedbackType.subtype = feedbackType.param();
                        mappedPayloadType.feedbackTypes.push_back(std::move(mappedFeedbackType));
                    }
                    for (const auto &[fst, snd] : codec.params) {
                        mappedPayloadType.parameters.emplace_back(fst, snd);
                    }
                    std::ranges::sort(mappedPayloadType.parameters, [](std::pair<std::string, std::string> const &lhs, std::pair<std::string, std::string> const &rhs) -> bool {
                        return lhs.first < rhs.first;
                    });
                    mappedContent.payloadTypes.push_back(std::move(mappedPayloadType));
                }
                break;
            case webrtc::MediaType::VIDEO:
                mappedContent.type = MediaContent::Type::Video;
                for (const auto &codec : content.media_description()->as_video()->codecs()) {
                    PayloadType mappedPayloadType;
                    mappedPayloadType.id = codec.id;
                    mappedPayloadType.name = codec.name;
                    mappedPayloadType.clockrate = codec.clockrate;
                    mappedPayloadType.channels = 0;
                    for (const auto &feedbackType : codec.feedback_params.params()) {
                        FeedbackType mappedFeedbackType;
                        mappedFeedbackType.type = feedbackType.id();
                        mappedFeedbackType.subtype = feedbackType.param();
                        mappedPayloadType.feedbackTypes.push_back(std::move(mappedFeedbackType));
                    }
                    for (const auto &[fst, snd] : codec.params) {
                        mappedPayloadType.parameters.emplace_back(fst, snd);
                    }
                    std::ranges::sort(mappedPayloadType.parameters, [](std::pair<std::string, std::string> const &lhs, std::pair<std::string, std::string> const &rhs) -> bool {
                        return lhs.first < rhs.first;
                    });
                    mappedContent.payloadTypes.push_back(std::move(mappedPayloadType));
                }
                break;
            default:
                throw RTCException("Unknown media type");
        }
        if (!content.media_description()->streams().empty()) {
            mappedContent.ssrc = content.media_description()->streams()[0].first_ssrc();
            for (const auto &ssrcGroup : content.media_description()->streams()[0].ssrc_groups) {
                SsrcGroup mappedSsrcGroup;
                mappedSsrcGroup.semantics = ssrcGroup.semantics;
                mappedSsrcGroup.ssrcs = ssrcGroup.ssrcs;
                mappedContent.ssrcGroups.push_back(std::move(mappedSsrcGroup));
            }
        }
        for (const auto &extension : content.media_description()->rtp_header_extensions()) {
            mappedContent.rtpExtensions.push_back(extension);
        }
        return mappedContent;
    }

    webrtc::ContentInfo ContentNegotiationContext::createInactiveContentInfo(std::string const &contentId) {
        auto audioDescription = std::make_unique<webrtc::AudioContentDescription>();
        auto contentDescription = std::move(audioDescription);
        contentDescription->set_direction(webrtc::RtpTransceiverDirection::kInactive);
        contentDescription->set_rtcp_mux(true);
        return {
            webrtc::MediaProtocolType::kRtp,
            contentId,
            std::move(contentDescription),
        };
    }

    webrtc::MediaDescriptionOptions ContentNegotiationContext::getIncomingContentDescription(const MediaContent &content) {
        auto mappedContent = convertSignalingContentToContentInfo(std::to_string(content.ssrc), content, webrtc::RtpTransceiverDirection::kSendOnly);
        webrtc::MediaDescriptionOptions contentDescription(mappedContent.media_description()->type(), mappedContent.mid(), webrtc::RtpTransceiverDirection::kRecvOnly, false);
        for (const auto &extension : mappedContent.media_description()->rtp_header_extensions()) {
            contentDescription.header_extensions.emplace_back(extension.uri, extension.id);
        }
        return contentDescription;
    }

    void ContentNegotiationContext::setAnswer(std::unique_ptr<NegotiationContents> &&answer) {
        if (!pendingOutgoingOffer) {
            return;
        }
        if (pendingOutgoingOffer->exchangeId != answer->exchangeId) {
            return;
        }

        pendingOutgoingOffer.reset();

        outgoingChannels.clear();

        for (const auto &content : answer->contents) {
            for (const auto &pendingChannel : outgoingChannelDescriptions) {
                if (pendingChannel.ssrc != 0 && content.ssrc == pendingChannel.ssrc) {
                    outgoingChannels.emplace_back(pendingChannel.description.mid, content);
                    break;
                }
            }
        }
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> ContentNegotiationContext::getAnswer(std::unique_ptr<NegotiationContents> &&offer) {
        auto currentSessionDescription = currentSessionDescriptionFromCoordinatedState();
        auto mappedOffer = std::make_unique<webrtc::SessionDescription>();
        webrtc::MediaSessionOptions answerOptions;
        answerOptions.offer_extmap_allow_mixed = true;
        answerOptions.bundle_enabled = true;
        for (const auto &id : channelIdOrder) {
            bool found = false;

            for (const auto &channel : outgoingChannels) {
                if (channel.id == id) {
                    found = true;
                    auto mappedContent = convertSignalingContentToContentInfo(channel.id, channel.content, webrtc::RtpTransceiverDirection::kRecvOnly);
                    webrtc::MediaDescriptionOptions contentDescription(mappedContent.media_description()->type(), mappedContent.mid(), webrtc::RtpTransceiverDirection::kSendOnly, false);
                    for (const auto &extension : mappedContent.media_description()->rtp_header_extensions()) {
                        contentDescription.header_extensions.emplace_back(extension.uri, extension.id);
                    }
                    answerOptions.media_description_options.push_back(contentDescription);
                    auto localCertificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                    if (localCertificate) {
                        fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                    }
                    std::vector<std::string> transportOptions;
                    webrtc::TransportDescription transportDescription(
                        transportOptions,
                        "ufrag",
                        "pwd",
                        webrtc::IceMode::ICEMODE_FULL,
                        webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    webrtc::TransportInfo transportInfo(channel.id, transportDescription);
                    mappedOffer->AddTransportInfo(transportInfo);
                    mappedOffer->AddContent(std::move(mappedContent));
                    break;
                }
            }
            for (const auto &content : offer->contents) {
                if (std::to_string(content.ssrc) == id) {
                    found = true;
                    auto mappedContent = convertSignalingContentToContentInfo(std::to_string(content.ssrc), content, webrtc::RtpTransceiverDirection::kSendOnly);
                    webrtc::MediaDescriptionOptions contentDescription(mappedContent.media_description()->type(), mappedContent.mid(), webrtc::RtpTransceiverDirection::kRecvOnly, false);
                    for (const auto &extension : mappedContent.media_description()->rtp_header_extensions()) {
                        contentDescription.header_extensions.emplace_back(extension.uri, extension.id);
                    }
                    answerOptions.media_description_options.push_back(contentDescription);
                    auto localCertificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                    if (localCertificate) {
                        fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                    }
                    std::vector<std::string> transportOptions;
                    webrtc::TransportDescription transportDescription(
                        transportOptions,
                        "ufrag",
                        "pwd",
                        webrtc::IceMode::ICEMODE_FULL,
                        webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    webrtc::TransportInfo transportInfo(mappedContent.mid(), transportDescription);
                    mappedOffer->AddTransportInfo(transportInfo);
                    mappedOffer->AddContent(std::move(mappedContent));
                    break;
                }
            }
            if (!found) {
                auto mappedContent = createInactiveContentInfo("_" + id);
                webrtc::MediaDescriptionOptions contentDescription(webrtc::MediaType::AUDIO, "_" + id, webrtc::RtpTransceiverDirection::kInactive, false);
                answerOptions.media_description_options.push_back(contentDescription);
                auto localCertificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                if (localCertificate) {
                    fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                }
                std::vector<std::string> transportOptions;
                webrtc::TransportDescription transportDescription(
                    transportOptions,
                    "ufrag",
                    "pwd",
                    webrtc::IceMode::ICEMODE_FULL,
                    webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                    fingerprint.get()
                );
                webrtc::TransportInfo transportInfo(mappedContent.mid(), transportDescription);
                mappedOffer->AddTransportInfo(transportInfo);
                mappedOffer->AddContent(std::move(mappedContent));
            }
        }
        for (const auto &content : offer->contents) {
            if (std::ranges::find(channelIdOrder, std::to_string(content.ssrc)) == channelIdOrder.end()) {
                channelIdOrder.push_back(std::to_string(content.ssrc));
                answerOptions.media_description_options.push_back(getIncomingContentDescription(content));
                auto mappedContent = convertSignalingContentToContentInfo(std::to_string(content.ssrc), content, webrtc::RtpTransceiverDirection::kSendOnly);
                auto localCertificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                if (localCertificate) {
                    fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                }

                std::vector<std::string> transportOptions;
                webrtc::TransportDescription transportDescription(
                    transportOptions,
                    "ufrag",
                    "pwd",
                    webrtc::IceMode::ICEMODE_FULL,
                    webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                    fingerprint.get()
                );
                webrtc::TransportInfo transportInfo(mappedContent.mid(), transportDescription);
                mappedOffer->AddTransportInfo(transportInfo);

                mappedOffer->AddContent(std::move(mappedContent));
            }
        }

        auto answerOrError = sessionDescriptionFactory->CreateAnswerOrError(mappedOffer.get(), answerOptions, currentSessionDescription.get());
        if (!answerOrError.ok()) {
            return nullptr;
        }
        auto answer = answerOrError.MoveValue();

        auto mappedAnswer = std::make_unique<NegotiationContents>();

        mappedAnswer->exchangeId = offer->exchangeId;

        std::vector<MediaContent> incomingChannels;

        for (const auto &content : answer->contents()) {
            auto mappedContent = convertContentInfoToSignalingContent(content);

            if (content.media_description()->direction() == webrtc::RtpTransceiverDirection::kRecvOnly) {
                for (const auto &[type, ssrc, ssrcGroups, payloadTypes, rtpExtensions] : offer->contents) {
                    if (std::to_string(ssrc) == content.mid()) {
                        mappedContent.ssrc = ssrc;
                        mappedContent.ssrcGroups = ssrcGroups;
                        break;
                    }
                }
                incomingChannels.push_back(mappedContent);
                mappedAnswer->contents.push_back(std::move(mappedContent));
            }
        }
        this->incomingChannels = incomingChannels;
        return mappedAnswer;
    }
} // wrtc