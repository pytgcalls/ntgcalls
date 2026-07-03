//
// Created by Lauren on 30/03/24.
//

#pragma once
#include <ntgcalls/signaling/messages/message.hpp>
#include <wrtc/models/media_content.hpp>

namespace ntgcalls::signaling::messages {
    class NegotiateChannelsMessage final: public Message {
        [[nodiscard]] static json serialize_content(const wrtc::models::MediaContent &content);

        [[nodiscard]] static json serialize_source_group(const wrtc::models::SsrcGroup& ssrc_group);

        [[nodiscard]] static json serialize_payload_type(const wrtc::models::PayloadType& payload_type);

        static wrtc::models::MediaContent deserialize_content(const json& content);

        static webrtc::RtpExtension deserialize_rtp_extension(const json& rtp_extension);

        static wrtc::models::FeedbackType deserialize_feedback_type(const json& feedback_type);

        static wrtc::models::SsrcGroup deserialize_source_group(const json& ssrc_group);

        static wrtc::models::PayloadType deserialize_payload_type(const json& payload_type);

    public:
        uint32_t exchange_id = 0;
        std::vector<wrtc::models::MediaContent> contents;

        [[nodiscard]] bytes::binary serialize() const override;

        static std::unique_ptr<NegotiateChannelsMessage> deserialize(const bytes::binary& data);
    };
} // ntgcalls::signaling