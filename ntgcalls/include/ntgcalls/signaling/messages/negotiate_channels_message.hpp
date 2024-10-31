//
// Created by Laky64 on 30/03/2024.
//

#pragma once
#include <wrtc/models/media_content.hpp>
#include <ntgcalls/signaling/messages/message.hpp>

namespace signaling {
    class NegotiateChannelsMessage final: public Message {
        [[nodiscard]] static json serializeContent(const wrtc::MediaContent &content);

        [[nodiscard]] static json serializeSourceGroup(const wrtc::SsrcGroup& ssrcGroup);

        [[nodiscard]] static json serializePayloadType(const wrtc::PayloadType& payloadType);

        static wrtc::MediaContent deserializeContent(const json& content);

        static webrtc::RtpExtension deserializeRtpExtension(const json& rtpExtension);

        static wrtc::FeedbackType deserializeFeedbackType(const json& feedbackType);

        static wrtc::SsrcGroup deserializeSourceGroup(const json& ssrcGroup);

        static wrtc::PayloadType deserializePayloadType(const json& payloadType);
    public:
        uint32_t exchangeId = 0;
        std::vector<wrtc::MediaContent> contents;

        [[nodiscard]] bytes::binary serialize() const override;

        static std::unique_ptr<NegotiateChannelsMessage> deserialize(const bytes::binary& data);
    };
} // signaling