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
        cricket::MediaEngineInterface *mediaEngine,
        rtc::UniqueRandomIdGenerator *uniqueRandomIdGenerator,
        webrtc::PayloadTypeSuggester *payloadTypeSuggester
    ) :isOutgoing(isOutgoing), uniqueRandomIdGenerator(uniqueRandomIdGenerator) {
        transportDescriptionFactory = std::make_unique<cricket::TransportDescriptionFactory>(fieldTrials);
        const auto tempCertificate = rtc::RTCCertificateGenerator::GenerateCertificate(
            rtc::KeyParams(rtc::KT_ECDSA),
            std::nullopt
        );
        transportDescriptionFactory->set_certificate(tempCertificate);
        sessionDescriptionFactory = std::make_unique<cricket::MediaSessionDescriptionFactory>(
            mediaEngine,
            true,
            uniqueRandomIdGenerator,
            transportDescriptionFactory.get(),
            payloadTypeSuggester
        );
        needNegotiation = true;
    }

    void ContentNegotiationContext::copyCodecsFromChannelManager(cricket::MediaEngineInterface *mediaEngine, const bool randomize) {
        std::vector<cricket::Codec> audioSendCodecs = mediaEngine->voice().send_codecs();
        std::vector<cricket::Codec> audioRecvCodecs = mediaEngine->voice().recv_codecs();
        std::vector<cricket::Codec> videoSendCodecs = mediaEngine->video().send_codecs();
        std::vector<cricket::Codec> videoRecvCodecs = mediaEngine->video().recv_codecs();
        for (const auto &codec : audioSendCodecs) {
            if (codec.name == "opus") {
                audioSendCodecs = { codec };
                audioRecvCodecs = { codec };
                break;
            }
        }
        if (randomize) {
            for (auto &codec : audioSendCodecs) {
                codec.id += 3;
            }
            for (auto &codec : videoSendCodecs) {
                codec.id += 3;
            }
            for (auto &codec : audioRecvCodecs) {
                codec.id += 3;
            }
            for (auto &codec : videoRecvCodecs) {
                codec.id += 3;
            }
        }
        sessionDescriptionFactory->set_audio_codecs(audioSendCodecs, audioRecvCodecs);
        sessionDescriptionFactory->set_video_codecs(videoSendCodecs, videoRecvCodecs);
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
        cricket::MediaType mappedMediaType;
        std::vector<webrtc::RtpHeaderExtensionCapability> rtpExtensions;
        if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
            mappedMediaType = cricket::MediaType::MEDIA_TYPE_AUDIO;
            rtpExtensions = rtpAudioExtensions;
        } else {
            mappedMediaType = cricket::MediaType::MEDIA_TYPE_VIDEO;
            rtpExtensions = rtpVideoExtensions;
        }
        cricket::MediaDescriptionOptions offerDescription(mappedMediaType, channelId, webrtc::RtpTransceiverDirection::kSendOnly, false);
        offerDescription.header_extensions = rtpExtensions;
        if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
            offerDescription.AddAudioSender(channelId, {channelId});
        } else {
            const cricket::SimulcastLayerList simulcastLayers;
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
        cricket::MediaSessionOptions offerOptions;
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
                cricket::MediaDescriptionOptions contentDescription(cricket::MediaType::MEDIA_TYPE_AUDIO, "_" + id, webrtc::RtpTransceiverDirection::kInactive, false);
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

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> ContentNegotiationContext::setPendingAnswer(std::unique_ptr<NegotiationContents>&& answer) {
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

    std::unique_ptr<cricket::SessionDescription> ContentNegotiationContext::currentSessionDescriptionFromCoordinatedState() const {
        if (channelIdOrder.empty()) {
            return nullptr;
        }
        auto sessionDescription = std::make_unique<cricket::SessionDescription>();
        for (const auto &id : channelIdOrder) {
            bool found = false;
            for (const auto &channel : incomingChannels) {
                if (std::to_string(channel.ssrc) == id) {
                    found = true;
                    auto mappedContent = convertSignalingContentToContentInfo(std::to_string(channel.ssrc), channel, webrtc::RtpTransceiverDirection::kRecvOnly);
                    auto localCertificate = rtc::RTCCertificateGenerator::GenerateCertificate(rtc::KeyParams(rtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<rtc::SSLFingerprint> fingerprint;
                    if (localCertificate) {
                        fingerprint = rtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                    }
                    std::vector<std::string> transportOptions;
                    cricket::TransportDescription transportDescription(
                        transportOptions,
                        "ufrag",
                        "pwd",
                        cricket::IceMode::ICEMODE_FULL,
                        cricket::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    cricket::TransportInfo transportInfo(std::to_string(channel.ssrc), transportDescription);
                    sessionDescription->AddTransportInfo(transportInfo);
                    sessionDescription->AddContent(std::move(mappedContent));
                    break;
                }
            }
            for (const auto &channel : outgoingChannels) {
                if (channel.id == id) {
                    found = true;
                    auto mappedContent = convertSignalingContentToContentInfo(channel.id, channel.content, webrtc::RtpTransceiverDirection::kSendOnly);
                    auto localCertificate = rtc::RTCCertificateGenerator::GenerateCertificate(rtc::KeyParams(rtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<rtc::SSLFingerprint> fingerprint;
                    if (localCertificate) {
                        fingerprint = rtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                    }
                    std::vector<std::string> transportOptions;
                    cricket::TransportDescription transportDescription(
                        transportOptions,
                        "ufrag",
                        "pwd",
                        cricket::IceMode::ICEMODE_FULL,
                        cricket::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    cricket::TransportInfo transportInfo(mappedContent.name, transportDescription);
                    sessionDescription->AddTransportInfo(transportInfo);
                    sessionDescription->AddContent(std::move(mappedContent));
                    break;
                }
            }

            if (!found) {
                auto mappedContent = createInactiveContentInfo("_" + id);
                auto localCertificate = rtc::RTCCertificateGenerator::GenerateCertificate(rtc::KeyParams(rtc::KT_ECDSA), std::nullopt);
                std::unique_ptr<rtc::SSLFingerprint> fingerprint;
                if (localCertificate) {
                    fingerprint = rtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                }
                std::vector<std::string> transportOptions;
                cricket::TransportDescription transportDescription(
                    transportOptions,
                    "ufrag",
                    "pwd",
                    cricket::IceMode::ICEMODE_FULL,
                    cricket::ConnectionRole::CONNECTIONROLE_ACTPASS,
                    fingerprint.get()
                );
                cricket::TransportInfo transportInfo(mappedContent.name, transportDescription);
                sessionDescription->AddTransportInfo(transportInfo);
                sessionDescription->AddContent(std::move(mappedContent));
            }
        }
        return sessionDescription;
    }

    cricket::ContentInfo ContentNegotiationContext::convertSignalingContentToContentInfo(const std::string &contentId, const MediaContent &content, webrtc::RtpTransceiverDirection direction) {
        std::unique_ptr<cricket::MediaContentDescription> contentDescription;
        switch (content.type) {
            case MediaContent::Type::Audio: {
                auto audioDescription = std::make_unique<cricket::AudioContentDescription>();
                for (const auto &[id, name, clockrate, channels, feedbackTypes, parameters] : content.payloadTypes) {
                    auto mappedCodec = cricket::CreateAudioCodec(
                        static_cast<int>(id),
                        name,
                        static_cast<int>(clockrate),
                        channels
                    );
                    for (const auto &parameter : parameters) {
                        mappedCodec.params.insert(parameter);
                    }
                    for (const auto &[type, subtype] : feedbackTypes) {
                        mappedCodec.AddFeedbackParam(cricket::FeedbackParam(type, subtype));
                    }
                    audioDescription->AddCodec(mappedCodec);
                }
                contentDescription = std::move(audioDescription);
                break;
            }
            case MediaContent::Type::Video: {
                auto videoDescription = std::make_unique<cricket::VideoContentDescription>();
                for (const auto &[id, name, clockrate, channels, feedbackTypes, parameters] : content.payloadTypes) {
                    webrtc::SdpVideoFormat videoFormat(name);
                    for (const auto &parameter : parameters) {
                        videoFormat.parameters.insert(parameter);
                    }
                    cricket::Codec mappedCodec = cricket::CreateVideoCodec(videoFormat);
                    mappedCodec.id = static_cast<int>(id);
                    for (const auto &parameter : parameters) {
                        mappedCodec.params.insert(parameter);
                    }
                    for (const auto &[type, subtype] : feedbackTypes) {
                        mappedCodec.AddFeedbackParam(cricket::FeedbackParam(type, subtype));
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
        cricket::StreamParams streamParams;
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
        cricket::ContentInfo mappedContentInfo(cricket::MediaProtocolType::kRtp);
        mappedContentInfo.name = contentId;
        mappedContentInfo.rejected = false;
        mappedContentInfo.bundle_only = false;
        mappedContentInfo.set_media_description(std::move(contentDescription));
        return mappedContentInfo;
    }

    MediaContent ContentNegotiationContext::convertContentInfoToSignalingContent(const cricket::ContentInfo &content) {
        MediaContent mappedContent;
        switch (content.media_description()->type()) {
            case cricket::MediaType::MEDIA_TYPE_AUDIO:
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
            case cricket::MediaType::MEDIA_TYPE_VIDEO:
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

    cricket::ContentInfo ContentNegotiationContext::createInactiveContentInfo(std::string const &contentId) {
        auto audioDescription = std::make_unique<cricket::AudioContentDescription>();
        auto contentDescription = std::move(audioDescription);
        contentDescription->set_direction(webrtc::RtpTransceiverDirection::kInactive);
        contentDescription->set_rtcp_mux(true);
        cricket::ContentInfo mappedContentInfo(cricket::MediaProtocolType::kRtp);
        mappedContentInfo.name = contentId;
        mappedContentInfo.rejected = false;
        mappedContentInfo.bundle_only = false;
        mappedContentInfo.set_media_description(std::move(contentDescription));
        return mappedContentInfo;
    }

    cricket::MediaDescriptionOptions ContentNegotiationContext::getIncomingContentDescription(const MediaContent &content) {
        auto mappedContent = convertSignalingContentToContentInfo(std::to_string(content.ssrc), content, webrtc::RtpTransceiverDirection::kSendOnly);
        cricket::MediaDescriptionOptions contentDescription(mappedContent.media_description()->type(), mappedContent.name, webrtc::RtpTransceiverDirection::kRecvOnly, false);
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
        auto mappedOffer = std::make_unique<cricket::SessionDescription>();
        cricket::MediaSessionOptions answerOptions;
        answerOptions.offer_extmap_allow_mixed = true;
        answerOptions.bundle_enabled = true;
        for (const auto &id : channelIdOrder) {
            bool found = false;

            for (const auto &channel : outgoingChannels) {
                if (channel.id == id) {
                    found = true;
                    auto mappedContent = convertSignalingContentToContentInfo(channel.id, channel.content, webrtc::RtpTransceiverDirection::kRecvOnly);
                    cricket::MediaDescriptionOptions contentDescription(mappedContent.media_description()->type(), mappedContent.name, webrtc::RtpTransceiverDirection::kSendOnly, false);
                    for (const auto &extension : mappedContent.media_description()->rtp_header_extensions()) {
                        contentDescription.header_extensions.emplace_back(extension.uri, extension.id);
                    }
                    answerOptions.media_description_options.push_back(contentDescription);
                    auto localCertificate = rtc::RTCCertificateGenerator::GenerateCertificate(rtc::KeyParams(rtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<rtc::SSLFingerprint> fingerprint;
                    if (localCertificate) {
                        fingerprint = rtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                    }
                    std::vector<std::string> transportOptions;
                    cricket::TransportDescription transportDescription(
                        transportOptions,
                        "ufrag",
                        "pwd",
                        cricket::IceMode::ICEMODE_FULL,
                        cricket::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    cricket::TransportInfo transportInfo(channel.id, transportDescription);
                    mappedOffer->AddTransportInfo(transportInfo);
                    mappedOffer->AddContent(std::move(mappedContent));
                    break;
                }
            }
            for (const auto &content : offer->contents) {
                if (std::to_string(content.ssrc) == id) {
                    found = true;
                    auto mappedContent = convertSignalingContentToContentInfo(std::to_string(content.ssrc), content, webrtc::RtpTransceiverDirection::kSendOnly);
                    cricket::MediaDescriptionOptions contentDescription(mappedContent.media_description()->type(), mappedContent.name, webrtc::RtpTransceiverDirection::kRecvOnly, false);
                    for (const auto &extension : mappedContent.media_description()->rtp_header_extensions()) {
                        contentDescription.header_extensions.emplace_back(extension.uri, extension.id);
                    }
                    answerOptions.media_description_options.push_back(contentDescription);
                    auto localCertificate = rtc::RTCCertificateGenerator::GenerateCertificate(rtc::KeyParams(rtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<rtc::SSLFingerprint> fingerprint;
                    if (localCertificate) {
                        fingerprint = rtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                    }
                    std::vector<std::string> transportOptions;
                    cricket::TransportDescription transportDescription(
                        transportOptions,
                        "ufrag",
                        "pwd",
                        cricket::IceMode::ICEMODE_FULL,
                        cricket::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    cricket::TransportInfo transportInfo(mappedContent.mid(), transportDescription);
                    mappedOffer->AddTransportInfo(transportInfo);
                    mappedOffer->AddContent(std::move(mappedContent));
                    break;
                }
            }
            if (!found) {
                auto mappedContent = createInactiveContentInfo("_" + id);
                cricket::MediaDescriptionOptions contentDescription(cricket::MediaType::MEDIA_TYPE_AUDIO, "_" + id, webrtc::RtpTransceiverDirection::kInactive, false);
                answerOptions.media_description_options.push_back(contentDescription);
                auto localCertificate = rtc::RTCCertificateGenerator::GenerateCertificate(rtc::KeyParams(rtc::KT_ECDSA), std::nullopt);
                std::unique_ptr<rtc::SSLFingerprint> fingerprint;
                if (localCertificate) {
                    fingerprint = rtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                }
                std::vector<std::string> transportOptions;
                cricket::TransportDescription transportDescription(
                    transportOptions,
                    "ufrag",
                    "pwd",
                    cricket::IceMode::ICEMODE_FULL,
                    cricket::ConnectionRole::CONNECTIONROLE_ACTPASS,
                    fingerprint.get()
                );
                cricket::TransportInfo transportInfo(mappedContent.mid(), transportDescription);
                mappedOffer->AddTransportInfo(transportInfo);
                mappedOffer->AddContent(std::move(mappedContent));
            }
        }
        for (const auto &content : offer->contents) {
            if (std::ranges::find(channelIdOrder, std::to_string(content.ssrc)) == channelIdOrder.end()) {
                channelIdOrder.push_back(std::to_string(content.ssrc));
                answerOptions.media_description_options.push_back(getIncomingContentDescription(content));
                auto mappedContent = convertSignalingContentToContentInfo(std::to_string(content.ssrc), content, webrtc::RtpTransceiverDirection::kSendOnly);
                auto localCertificate = rtc::RTCCertificateGenerator::GenerateCertificate(rtc::KeyParams(rtc::KT_ECDSA), std::nullopt);
                std::unique_ptr<rtc::SSLFingerprint> fingerprint;
                if (localCertificate) {
                    fingerprint = rtc::SSLFingerprint::CreateFromCertificate(*localCertificate);
                }

                std::vector<std::string> transportOptions;
                cricket::TransportDescription transportDescription(
                    transportOptions,
                    "ufrag",
                    "pwd",
                    cricket::IceMode::ICEMODE_FULL,
                    cricket::ConnectionRole::CONNECTIONROLE_ACTPASS,
                    fingerprint.get()
                );
                cricket::TransportInfo transportInfo(mappedContent.mid(), transportDescription);
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