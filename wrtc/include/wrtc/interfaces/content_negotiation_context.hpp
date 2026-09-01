//
// Created by Lauren on 30/03/24.
//

#pragma once
#include <p2p/base/transport_description_factory.h>
#include <pc/media_session.h>
#include <rtc_base/unique_id_generator.h>
#include <wrtc/interfaces/media/codec_lookup_helper.hpp>
#include <wrtc/models/media_content.hpp>

namespace wrtc::interfaces {

    class ContentNegotiationContext {
    public:
        struct OutgoingChannel {
            std::string id;
            models::MediaContent content;

            OutgoingChannel(std::string id, models::MediaContent content): id(std::move(id)), content(std::move(content)) {}
        };

        struct PendingOutgoingChannel {
            webrtc::MediaDescriptionOptions description;
            uint32_t ssrc = 0;
            std::vector<models::SsrcGroup> ssrc_groups;
            explicit PendingOutgoingChannel(webrtc::MediaDescriptionOptions&& description): description(std::move(description)) {}
        };

        struct NegotiationContents {
            uint32_t exchange_id = 0;
            std::vector<models::MediaContent> contents;
        };

        struct PendingOutgoingOffer {
            uint32_t exchange_id = 0;
        };

        struct CoordinatedState {
            std::vector<models::MediaContent> outgoing_contents;
            std::vector<models::MediaContent> incoming_contents;
        };

    private:
        bool is_outgoing_, need_negotiation_;
        webrtc::UniqueRandomIdGenerator* unique_random_id_generator_;
        std::unique_ptr<webrtc::TransportDescriptionFactory> transport_description_factory_;
        std::unique_ptr<webrtc::MediaSessionDescriptionFactory> session_description_factory_;
        std::vector<webrtc::RtpHeaderExtensionCapability> rtp_audio_extensions_;
        std::vector<webrtc::RtpHeaderExtensionCapability> rtp_video_extensions_;
        std::vector<PendingOutgoingChannel> outgoing_channel_descriptions_;
        std::unique_ptr<PendingOutgoingOffer> pending_outgoing_offer_;
        std::unique_ptr<media::CodecLookupHelper> codec_lookup_helper_;
        std::vector<std::string> channel_id_order_;
        std::vector<models::MediaContent> incoming_channels_;
        std::vector<OutgoingChannel> outgoing_channels_;
        int next_outgoing_channel_id_ = 0;

        [[nodiscard]] std::unique_ptr<webrtc::SessionDescription> current_session_description_from_coordinated_state() const;

        static webrtc::ContentInfo convert_signaling_content_to_content_info(const std::string& content_id, const models::MediaContent& content, webrtc::RtpTransceiverDirection direction);

        static models::MediaContent convert_content_info_to_signaling_content(const webrtc::ContentInfo& content);

        static webrtc::ContentInfo create_inactive_content_info(const std::string& content_id);

        static webrtc::MediaDescriptionOptions get_incoming_content_description(const models::MediaContent& content);

        void set_answer(std::unique_ptr<NegotiationContents>&& answer);

        std::unique_ptr<NegotiationContents> get_answer(std::unique_ptr<NegotiationContents>&& offer);

    public:
        ContentNegotiationContext(
            const webrtc::Environment& env,
            bool is_outgoing,
            webrtc::MediaEngineInterface* media_engine,
            webrtc::UniqueRandomIdGenerator* unique_random_id_generator,
            webrtc::PayloadTypeSuggester* payload_type_suggester
        );

        ~ContentNegotiationContext();

        void copy_codecs_from_channel_manager(bool randomize);

        std::string add_outgoing_channel(const webrtc::MediaStreamTrackInterface* track);

        std::unique_ptr<NegotiationContents> get_pending_offer();

        std::unique_ptr<NegotiationContents> set_pending_answer(std::unique_ptr<NegotiationContents> answer);

        [[nodiscard]] std::unique_ptr<CoordinatedState> coordinated_state() const;

        [[nodiscard]] std::optional<uint32_t> outgoing_channel_ssrc(const std::string& id) const;
    };

} // wrtc::interfaces
