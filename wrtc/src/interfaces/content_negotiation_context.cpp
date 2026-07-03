//
// Created by Lauren on 30/03/24.
//

#include <media/base/media_engine.h>
#include <p2p/base/transport_description.h>
#include <pc/webrtc_session_description_factory.h>
#include <rtc_base/rtc_certificate_generator.h>
#include <wrtc/exceptions.hpp>
#include <wrtc/interfaces/content_negotiation_context.hpp>

namespace wrtc::interfaces {
    ContentNegotiationContext::ContentNegotiationContext(
        const webrtc::Environment& env,
        const bool is_outgoing,
        webrtc::MediaEngineInterface* media_engine,
        webrtc::UniqueRandomIdGenerator* unique_random_id_generator,
        webrtc::PayloadTypeSuggester* payload_type_suggester
    ) :is_outgoing_(is_outgoing), unique_random_id_generator_(unique_random_id_generator) {
        transport_description_factory_ = std::make_unique<webrtc::TransportDescriptionFactory>(env.field_trials());
        const auto temp_certificate = webrtc::RTCCertificateGenerator::GenerateCertificate(
            webrtc::KeyParams(webrtc::KT_ECDSA),
            std::nullopt
        );
        transport_description_factory_->set_certificate(temp_certificate);
        codec_lookup_helper_ = std::make_unique<media::CodecLookupHelper>(
            media_engine,
            transport_description_factory_.get(),
            payload_type_suggester
        );
        session_description_factory_ = std::make_unique<webrtc::MediaSessionDescriptionFactory>(
            env,
            media_engine,
            true,
            unique_random_id_generator,
            transport_description_factory_.get(),
            nullptr,
            codec_lookup_helper_.get()
        );
        need_negotiation_ = true;
    }

    ContentNegotiationContext::~ContentNegotiationContext() {
        session_description_factory_ = nullptr;
        transport_description_factory_ = nullptr;
        codec_lookup_helper_ = nullptr;
        unique_random_id_generator_ = nullptr;
        outgoing_channel_descriptions_.clear();
        channel_id_order_.clear();
        incoming_channels_.clear();
        outgoing_channels_.clear();
        rtp_audio_extensions_.clear();
        rtp_video_extensions_.clear();
        pending_outgoing_offer_.reset();
    }

    void ContentNegotiationContext::copy_codecs_from_channel_manager(const bool randomize) {
        int abs_send_time_uri_id = 2;
        int transport_sequence_number_uri_id = 3;
        int video_rotation_uri = 13;

        if (randomize) {
            abs_send_time_uri_id = 3;
            transport_sequence_number_uri_id = 2;
            video_rotation_uri = 4;
        }
        rtp_audio_extensions_.emplace_back(webrtc::RtpExtension::kAbsSendTimeUri, abs_send_time_uri_id);
        rtp_audio_extensions_.emplace_back(webrtc::RtpExtension::kTransportSequenceNumberUri, transport_sequence_number_uri_id);
        rtp_video_extensions_.emplace_back(webrtc::RtpExtension::kAudioLevelUri, 1);
        rtp_video_extensions_.emplace_back(webrtc::RtpExtension::kAbsSendTimeUri, abs_send_time_uri_id);
        rtp_video_extensions_.emplace_back(webrtc::RtpExtension::kTransportSequenceNumberUri, transport_sequence_number_uri_id);
        rtp_video_extensions_.emplace_back(webrtc::RtpExtension::kVideoRotationUri, video_rotation_uri);
    }

    std::string ContentNegotiationContext::add_outgoing_channel(const webrtc::MediaStreamTrackInterface* track) {
        std::string channel_id = track->id();
        webrtc::MediaType mapped_media_type;
        std::vector<webrtc::RtpHeaderExtensionCapability> rtp_extensions;
        if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
            mapped_media_type = webrtc::MediaType::AUDIO;
            rtp_extensions = rtp_audio_extensions_;
        } else {
            mapped_media_type = webrtc::MediaType::VIDEO;
            rtp_extensions = rtp_video_extensions_;
        }
        webrtc::MediaDescriptionOptions offer_description(mapped_media_type, channel_id, webrtc::RtpTransceiverDirection::kSendOnly, false);
        offer_description.header_extensions = rtp_extensions;
        if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
            offer_description.AddAudioSender(channel_id, {channel_id});
        } else {
            const webrtc::SimulcastLayerList simulcast_layers;
            offer_description.AddVideoSender(channel_id, {channel_id}, {}, simulcast_layers, 1);
        }
        outgoing_channel_descriptions_.emplace_back(std::move(offer_description));
        need_negotiation_ = true;
        return channel_id;
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> ContentNegotiationContext::get_pending_offer() {
        if (!need_negotiation_) {
            return nullptr;
        }
        if (pending_outgoing_offer_) {
            return nullptr;
        }
        need_negotiation_ = false;
        pending_outgoing_offer_ = std::make_unique<PendingOutgoingOffer>();
        pending_outgoing_offer_->exchange_id = unique_random_id_generator_->GenerateId();
        auto current_session_description = current_session_description_from_coordinated_state();
        webrtc::MediaSessionOptions offer_options;
        offer_options.offer_extmap_allow_mixed = true;
        offer_options.bundle_enabled = true;
        for (const auto &id : channel_id_order_) {
            bool found = false;
            for (const auto &channel : outgoing_channel_descriptions_) {
                if (channel.description.mid == id) {
                    found = true;
                    offer_options.media_description_options.push_back(channel.description);

                    break;
                }
            }
            for (const auto &content : incoming_channels_) {
                if (std::to_string(content.ssrc) == id) {
                    found = true;
                    offer_options.media_description_options.push_back(get_incoming_content_description(content));
                    break;
                }
            }
            if (!found) {
                const webrtc::MediaDescriptionOptions content_description(webrtc::MediaType::AUDIO, "_" + id, webrtc::RtpTransceiverDirection::kInactive, false);
                offer_options.media_description_options.push_back(content_description);
            }
        }
        for (const auto &channel : outgoing_channel_descriptions_) {
            if (std::ranges::find(channel_id_order_, channel.description.mid) == channel_id_order_.end()) {
                channel_id_order_.push_back(channel.description.mid);
                offer_options.media_description_options.push_back(channel.description);
            }
            for (const auto &content : incoming_channels_) {
                if (std::ranges::find(channel_id_order_, std::to_string(content.ssrc)) == channel_id_order_.end()) {
                    channel_id_order_.push_back(std::to_string(content.ssrc));
                    offer_options.media_description_options.push_back(get_incoming_content_description(content));
                }
            }
        }
        auto offer_or_error = session_description_factory_->CreateOfferOrError(
            offer_options, current_session_description.get()
        );
        if (!offer_or_error.ok()) {
            RTC_LOG(LS_ERROR) << "Failed to create offer: " << offer_or_error.error().message();
            wrap_rtc_error(offer_or_error.error());
        }
        auto offer = offer_or_error.MoveValue();
        auto mapped_offer = std::make_unique<NegotiationContents>();
        mapped_offer->exchange_id = pending_outgoing_offer_->exchange_id;
        for (const auto &content : offer->contents()) {
            auto mapped_content = convert_content_info_to_signaling_content(content);
            if (content.media_description()->direction() == webrtc::RtpTransceiverDirection::kSendOnly) {
                mapped_offer->contents.push_back(std::move(mapped_content));
                for (auto &channel : outgoing_channel_descriptions_) {
                    if (channel.description.mid == content.mid()) {
                        channel.ssrc = mapped_content.ssrc;
                        channel.ssrc_groups = mapped_content.ssrc_groups;
                    }
                }
            }
        }
        return mapped_offer;
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> ContentNegotiationContext::set_pending_answer(std::unique_ptr<NegotiationContents> answer) {
        if (!answer) {
            return nullptr;
        }
        if (pending_outgoing_offer_) {
            if (answer->exchange_id == pending_outgoing_offer_->exchange_id) {
                set_answer(std::move(answer));
                return nullptr;
            }
            if (!is_outgoing_) {
                pending_outgoing_offer_.reset();
                return get_answer(std::move(answer));
            }
            return nullptr;
        }
        return get_answer(std::move(answer));
    }

    std::unique_ptr<ContentNegotiationContext::CoordinatedState> ContentNegotiationContext::coordinated_state() const {
        auto result = std::make_unique<CoordinatedState>();
        result->incoming_contents = incoming_channels_;
        for (const auto &channel : outgoing_channels_) {
            bool found = false;
            for (const auto &channel_description : outgoing_channel_descriptions_) {
                if (channel_description.description.mid == channel.id) {
                    found = true;
                    break;
                }
            }
            if (found) {
                result->outgoing_contents.push_back(channel.content);
            }
        }
        return result;
    }

    std::optional<uint32_t> ContentNegotiationContext::outgoing_channel_ssrc(const std::string &id) const {
        for (const auto &channel : outgoing_channels_) {
            bool found = false;
            for (const auto &channel_description : outgoing_channel_descriptions_) {
                if (channel_description.description.mid == channel.id) {
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

    std::unique_ptr<webrtc::SessionDescription> ContentNegotiationContext::current_session_description_from_coordinated_state() const {
        if (channel_id_order_.empty()) {
            return nullptr;
        }
        auto session_description = std::make_unique<webrtc::SessionDescription>();
        for (const auto &id : channel_id_order_) {
            bool found = false;
            for (const auto &channel : incoming_channels_) {
                if (std::to_string(channel.ssrc) == id) {
                    found = true;
                    auto mapped_content = convert_signaling_content_to_content_info(std::to_string(channel.ssrc), channel, webrtc::RtpTransceiverDirection::kRecvOnly);
                    auto local_certificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                    if (local_certificate) {
                        fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*local_certificate);
                    }
                    const webrtc::TransportDescription transport_description(
                        {},
                        "ufrag",
                        "pwd",
                        webrtc::IceMode::ICEMODE_FULL,
                        webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    const webrtc::TransportInfo transport_info(std::to_string(channel.ssrc), transport_description);
                    session_description->AddTransportInfo(transport_info);
                    session_description->AddContent(std::move(mapped_content));
                    break;
                }
            }
            for (const auto &channel : outgoing_channels_) {
                if (channel.id == id) {
                    found = true;
                    auto mapped_content = convert_signaling_content_to_content_info(channel.id, channel.content, webrtc::RtpTransceiverDirection::kSendOnly);
                    auto local_certificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                    if (local_certificate) {
                        fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*local_certificate);
                    }
                    const webrtc::TransportDescription transport_description(
                        {},
                        "ufrag",
                        "pwd",
                        webrtc::IceMode::ICEMODE_FULL,
                        webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    const webrtc::TransportInfo transport_info(mapped_content.mid(), transport_description);
                    session_description->AddTransportInfo(transport_info);
                    session_description->AddContent(std::move(mapped_content));
                    break;
                }
            }

            if (!found) {
                auto mapped_content = create_inactive_content_info("_" + id);
                auto local_certificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                if (local_certificate) {
                    fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*local_certificate);
                }
                const webrtc::TransportDescription transport_description(
                    {},
                    "ufrag",
                    "pwd",
                    webrtc::IceMode::ICEMODE_FULL,
                    webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                    fingerprint.get()
                );
                const webrtc::TransportInfo transport_info(mapped_content.mid(), transport_description);
                session_description->AddTransportInfo(transport_info);
                session_description->AddContent(std::move(mapped_content));
            }
        }
        return session_description;
    }

    webrtc::ContentInfo ContentNegotiationContext::convert_signaling_content_to_content_info(const std::string &content_id, const models::MediaContent &content, webrtc::RtpTransceiverDirection direction) {
        std::unique_ptr<webrtc::MediaContentDescription> content_description;
        switch (content.type) {
            case models::MediaContent::Type::Audio: {
                auto audio_description = std::make_unique<webrtc::AudioContentDescription>();
                for (const auto &[id, name, clockrate, channels, feedback_types, parameters] : content.payload_types) {
                    auto mapped_codec = webrtc::CreateAudioCodec(
                        id,
                        name,
                        clockrate,
                        channels
                    );
                    for (const auto &parameter : parameters) {
                        mapped_codec.params.insert(parameter);
                    }
                    for (const auto &[type, subtype] : feedback_types) {
                        mapped_codec.AddFeedbackParam(webrtc::FeedbackParam(type, subtype));
                    }
                    audio_description->AddCodec(mapped_codec);
                }
                content_description = std::move(audio_description);
                break;
            }
            case models::MediaContent::Type::Video: {
                auto video_description = std::make_unique<webrtc::VideoContentDescription>();
                for (const auto &[id, name, clockrate, channels, feedback_types, parameters] : content.payload_types) {
                    webrtc::SdpVideoFormat video_format(name);
                    for (const auto &parameter : parameters) {
                        video_format.parameters.insert(parameter);
                    }
                    webrtc::Codec mapped_codec = webrtc::CreateVideoCodec(video_format);
                    mapped_codec.id = id;
                    for (const auto &parameter : parameters) {
                        mapped_codec.params.insert(parameter);
                    }
                    for (const auto &[type, subtype] : feedback_types) {
                        mapped_codec.AddFeedbackParam(webrtc::FeedbackParam(type, subtype));
                    }
                    video_description->AddCodec(mapped_codec);
                }
                content_description = std::move(video_description);
                break;
            }
            default: {
                throw RTCException("Unknown media type");
            }
        }
        webrtc::StreamParams stream_params;
        stream_params.id = content_id;
        stream_params.set_stream_ids({ content_id });
        stream_params.add_ssrc(content.ssrc);
        for (const auto &[semantics, ssrcs] : content.ssrc_groups) {
            stream_params.ssrc_groups.emplace_back(semantics, ssrcs);
            for (const auto &ssrc : ssrcs) {
                if (!stream_params.has_ssrc(ssrc)) {
                    stream_params.add_ssrc(ssrc);
                }
            }
        }
        content_description->AddStream(stream_params);
        for (const auto &extension : content.rtp_extensions) {
            content_description->AddRtpHeaderExtension(extension);
        }
        content_description->set_direction(direction);
        content_description->set_rtcp_mux(true);
        return {
            webrtc::MediaProtocolType::kRtp,
            content_id,
            std::move(content_description),
        };
    }

    models::MediaContent ContentNegotiationContext::convert_content_info_to_signaling_content(const webrtc::ContentInfo &content) {
        models::MediaContent mapped_content;
        switch (content.media_description()->type()) {
            case webrtc::MediaType::AUDIO:
                mapped_content.type = models::MediaContent::Type::Audio;
                for (const auto &codec : content.media_description()->as_audio()->codecs()) {
                    models::PayloadType mapped_payload_type;
                    mapped_payload_type.id = codec.id;
                    mapped_payload_type.name = codec.name;
                    mapped_payload_type.clockrate = codec.clockrate;
                    mapped_payload_type.channels = codec.channels;
                    for (const auto &feedback_type : codec.feedback_params.params()) {
                        models::FeedbackType mapped_feedback_type;
                        mapped_feedback_type.type = feedback_type.id();
                        mapped_feedback_type.subtype = feedback_type.param();
                        mapped_payload_type.feedback_types.push_back(std::move(mapped_feedback_type));
                    }
                    for (const auto &[fst, snd] : codec.params) {
                        mapped_payload_type.parameters.emplace_back(fst, snd);
                    }
                    std::ranges::sort(mapped_payload_type.parameters, [](std::pair<std::string, std::string> const &lhs, std::pair<std::string, std::string> const &rhs) -> bool {
                        return lhs.first < rhs.first;
                    });
                    mapped_content.payload_types.push_back(std::move(mapped_payload_type));
                }
                break;
            case webrtc::MediaType::VIDEO:
                mapped_content.type = models::MediaContent::Type::Video;
                for (const auto &codec : content.media_description()->as_video()->codecs()) {
                    models::PayloadType mapped_payload_type;
                    mapped_payload_type.id = codec.id;
                    mapped_payload_type.name = codec.name;
                    mapped_payload_type.clockrate = codec.clockrate;
                    mapped_payload_type.channels = 0;
                    for (const auto &feedback_type : codec.feedback_params.params()) {
                        models::FeedbackType mapped_feedback_type;
                        mapped_feedback_type.type = feedback_type.id();
                        mapped_feedback_type.subtype = feedback_type.param();
                        mapped_payload_type.feedback_types.push_back(std::move(mapped_feedback_type));
                    }
                    for (const auto &[fst, snd] : codec.params) {
                        mapped_payload_type.parameters.emplace_back(fst, snd);
                    }
                    std::ranges::sort(mapped_payload_type.parameters, [](std::pair<std::string, std::string> const &lhs, std::pair<std::string, std::string> const &rhs) -> bool {
                        return lhs.first < rhs.first;
                    });
                    mapped_content.payload_types.push_back(std::move(mapped_payload_type));
                }
                break;
            default:
                throw RTCException("Unknown media type");
        }
        if (!content.media_description()->streams().empty()) {
            mapped_content.ssrc = content.media_description()->streams()[0].first_ssrc();
            for (const auto &ssrc_group : content.media_description()->streams()[0].ssrc_groups) {
                models::SsrcGroup mapped_ssrc_group;
                mapped_ssrc_group.semantics = ssrc_group.semantics;
                mapped_ssrc_group.ssrcs = ssrc_group.ssrcs;
                mapped_content.ssrc_groups.push_back(std::move(mapped_ssrc_group));
            }
        }
        for (const auto &extension : content.media_description()->rtp_header_extensions()) {
            mapped_content.rtp_extensions.push_back(extension);
        }
        return mapped_content;
    }

    webrtc::ContentInfo ContentNegotiationContext::create_inactive_content_info(std::string const &content_id) {
        auto audio_description = std::make_unique<webrtc::AudioContentDescription>();
        auto content_description = std::move(audio_description);
        content_description->set_direction(webrtc::RtpTransceiverDirection::kInactive);
        content_description->set_rtcp_mux(true);
        return {
            webrtc::MediaProtocolType::kRtp,
            content_id,
            std::move(content_description),
        };
    }

    webrtc::MediaDescriptionOptions ContentNegotiationContext::get_incoming_content_description(const models::MediaContent &content) {
        auto mapped_content = convert_signaling_content_to_content_info(std::to_string(content.ssrc), content, webrtc::RtpTransceiverDirection::kSendOnly);
        webrtc::MediaDescriptionOptions content_description(mapped_content.media_description()->type(), mapped_content.mid(), webrtc::RtpTransceiverDirection::kRecvOnly, false);
        for (const auto &extension : mapped_content.media_description()->rtp_header_extensions()) {
            content_description.header_extensions.emplace_back(extension.uri, extension.id);
        }
        return content_description;
    }

    void ContentNegotiationContext::set_answer(std::unique_ptr<NegotiationContents> &&answer) {
        if (!pending_outgoing_offer_) {
            return;
        }
        if (pending_outgoing_offer_->exchange_id != answer->exchange_id) {
            return;
        }

        pending_outgoing_offer_.reset();

        outgoing_channels_.clear();

        for (const auto &content : answer->contents) {
            for (const auto &pending_channel : outgoing_channel_descriptions_) {
                if (pending_channel.ssrc != 0 && content.ssrc == pending_channel.ssrc) {
                    outgoing_channels_.emplace_back(pending_channel.description.mid, content);
                    break;
                }
            }
        }
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> ContentNegotiationContext::get_answer(std::unique_ptr<NegotiationContents> &&offer) {
        auto current_session_description = current_session_description_from_coordinated_state();
        auto mapped_offer = std::make_unique<webrtc::SessionDescription>();
        webrtc::MediaSessionOptions answer_options;
        answer_options.offer_extmap_allow_mixed = true;
        answer_options.bundle_enabled = true;
        for (const auto &id : channel_id_order_) {
            bool found = false;

            for (const auto &channel : outgoing_channels_) {
                if (channel.id == id) {
                    found = true;
                    auto mapped_content = convert_signaling_content_to_content_info(channel.id, channel.content, webrtc::RtpTransceiverDirection::kRecvOnly);
                    webrtc::MediaDescriptionOptions content_description(mapped_content.media_description()->type(), mapped_content.mid(), webrtc::RtpTransceiverDirection::kSendOnly, false);
                    for (const auto &extension : mapped_content.media_description()->rtp_header_extensions()) {
                        content_description.header_extensions.emplace_back(extension.uri, extension.id);
                    }
                    answer_options.media_description_options.push_back(content_description);
                    auto local_certificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                    if (local_certificate) {
                        fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*local_certificate);
                    }
                    const webrtc::TransportDescription transport_description(
                        {},
                        "ufrag",
                        "pwd",
                        webrtc::IceMode::ICEMODE_FULL,
                        webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    const webrtc::TransportInfo transport_info(channel.id, transport_description);
                    mapped_offer->AddTransportInfo(transport_info);
                    mapped_offer->AddContent(std::move(mapped_content));
                    break;
                }
            }
            for (const auto &content : offer->contents) {
                if (std::to_string(content.ssrc) == id) {
                    found = true;
                    auto mapped_content = convert_signaling_content_to_content_info(std::to_string(content.ssrc), content, webrtc::RtpTransceiverDirection::kSendOnly);
                    webrtc::MediaDescriptionOptions content_description(mapped_content.media_description()->type(), mapped_content.mid(), webrtc::RtpTransceiverDirection::kRecvOnly, false);
                    for (const auto &extension : mapped_content.media_description()->rtp_header_extensions()) {
                        content_description.header_extensions.emplace_back(extension.uri, extension.id);
                    }
                    answer_options.media_description_options.push_back(content_description);
                    auto local_certificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                    std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                    if (local_certificate) {
                        fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*local_certificate);
                    }
                    const webrtc::TransportDescription transport_description(
                        {},
                        "ufrag",
                        "pwd",
                        webrtc::IceMode::ICEMODE_FULL,
                        webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                        fingerprint.get()
                    );
                    const webrtc::TransportInfo transport_info(mapped_content.mid(), transport_description);
                    mapped_offer->AddTransportInfo(transport_info);
                    mapped_offer->AddContent(std::move(mapped_content));
                    break;
                }
            }
            if (!found) {
                auto mapped_content = create_inactive_content_info("_" + id);
                const webrtc::MediaDescriptionOptions content_description(webrtc::MediaType::AUDIO, "_" + id, webrtc::RtpTransceiverDirection::kInactive, false);
                answer_options.media_description_options.push_back(content_description);
                auto local_certificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                if (local_certificate) {
                    fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*local_certificate);
                }
                const webrtc::TransportDescription transport_description(
                    {},
                    "ufrag",
                    "pwd",
                    webrtc::IceMode::ICEMODE_FULL,
                    webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                    fingerprint.get()
                );
                const webrtc::TransportInfo transport_info(mapped_content.mid(), transport_description);
                mapped_offer->AddTransportInfo(transport_info);
                mapped_offer->AddContent(std::move(mapped_content));
            }
        }
        for (const auto &content : offer->contents) {
            if (std::ranges::find(channel_id_order_, std::to_string(content.ssrc)) == channel_id_order_.end()) {
                channel_id_order_.push_back(std::to_string(content.ssrc));
                answer_options.media_description_options.push_back(get_incoming_content_description(content));
                auto mapped_content = convert_signaling_content_to_content_info(std::to_string(content.ssrc), content, webrtc::RtpTransceiverDirection::kSendOnly);
                auto local_certificate = webrtc::RTCCertificateGenerator::GenerateCertificate(webrtc::KeyParams(webrtc::KT_ECDSA), std::nullopt);
                std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                if (local_certificate) {
                    fingerprint = webrtc::SSLFingerprint::CreateFromCertificate(*local_certificate);
                }

                const webrtc::TransportDescription transport_description(
                    {},
                    "ufrag",
                    "pwd",
                    webrtc::IceMode::ICEMODE_FULL,
                    webrtc::ConnectionRole::CONNECTIONROLE_ACTPASS,
                    fingerprint.get()
                );
                const webrtc::TransportInfo transport_info(mapped_content.mid(), transport_description);
                mapped_offer->AddTransportInfo(transport_info);

                mapped_offer->AddContent(std::move(mapped_content));
            }
        }

        auto answer_or_error = session_description_factory_->CreateAnswerOrError(mapped_offer.get(), answer_options, current_session_description.get());
        if (!answer_or_error.ok()) {
            return nullptr;
        }
        auto answer = answer_or_error.MoveValue();

        auto mapped_answer = std::make_unique<NegotiationContents>();

        mapped_answer->exchange_id = offer->exchange_id;

        std::vector<models::MediaContent> temp_incoming_channels;

        for (const auto &content : answer->contents()) {
            auto mapped_content = convert_content_info_to_signaling_content(content);

            if (content.media_description()->direction() == webrtc::RtpTransceiverDirection::kRecvOnly) {
                for (const auto &[type, ssrc, userID, ssrc_groups, payload_types, rtp_extensions] : offer->contents) {
                    if (std::to_string(ssrc) == content.mid()) {
                        mapped_content.ssrc = ssrc;
                        mapped_content.ssrc_groups = ssrc_groups;
                        mapped_content.user_id = userID;
                        break;
                    }
                }
                temp_incoming_channels.push_back(mapped_content);
                mapped_answer->contents.push_back(std::move(mapped_content));
            }
        }
        incoming_channels_ = temp_incoming_channels;
        return mapped_answer;
    }
} // wrtc::interfaces