//
// Created by Laky64 on 30/03/2024.
//

#include <ntgcalls/signaling/messages/negotiate_channels_message.hpp>

#include <ntgcalls/exceptions.hpp>

namespace signaling {
    json NegotiateChannelsMessage::serializeSourceGroup(const wrtc::SsrcGroup &ssrcGroup) {
        auto ssrcsJson = json::array();
        for (const auto ssrc : ssrcGroup.ssrcs) {
            ssrcsJson.push_back(std::to_string(ssrc));
        }
        return {
            {"semantics", ssrcGroup.semantics},
            {"ssrcs", ssrcsJson},
        };
    }

    json NegotiateChannelsMessage::serializePayloadType(const wrtc::PayloadType &payloadType) {
        auto res = json{
            {"id", payloadType.id},
            {"name", payloadType.name},
            {"clockrate", payloadType.clockrate},
            {"channels", payloadType.channels},
        };
        auto feedbackTypesJson = json::array();
        if (!payloadType.feedbackTypes.empty()) {
            for (const auto&[type, subtype] : payloadType.feedbackTypes) {
                feedbackTypesJson.push_back({
                    {"type", type},
                    {"subtype", subtype},
                });
            }
        }
        res["feedbackTypes"] = feedbackTypesJson;
        auto parametersJson = json::object();
        for (const auto& [key, value] : payloadType.parameters) {
            parametersJson[key] = value;
        }
        res["parameters"] = parametersJson;
        return res;
    }

    json NegotiateChannelsMessage::serializeContent(const wrtc::MediaContent &content) {
        json contentJson = {
            {"type", content.type == wrtc::MediaContent::Type::Audio ? "audio" : "video"},
            {"ssrc", std::to_string(content.ssrc)},
        };
        if (!content.ssrcGroups.empty()) {
            auto ssrcGroupsJson = json::array();
            for (const auto& ssrcGroup : content.ssrcGroups) {
                ssrcGroupsJson.push_back(serializeSourceGroup(ssrcGroup));
            }
            contentJson["ssrcGroups"] = ssrcGroupsJson;
        }
        if (!content.payloadTypes.empty()) {
            auto payloadTypesJson = json::array();
            for (const auto& payloadType : content.payloadTypes) {
                payloadTypesJson.push_back(serializePayloadType(payloadType));
            }
            contentJson["payloadTypes"] = payloadTypesJson;
        }
        auto rtpExtensionsJson = json::array();
        for (const auto& rtpExtension : content.rtpExtensions) {
            rtpExtensionsJson.push_back(json{
                {"uri", rtpExtension.uri},
                {"id", rtpExtension.id},
            });
        }
        contentJson["rtpExtensions"] = rtpExtensionsJson;
        return contentJson;
    }

    bytes::binary NegotiateChannelsMessage::serialize() const {
        auto res = json{
            {"@type", "NegotiateChannels"},
            {"exchangeId", std::to_string(exchangeId)},
        };
        auto contentsJson = json::array();
        for (const auto& content : contents) {
            contentsJson.push_back(serializeContent(content));
        }
        res["contents"] = contentsJson;
        return bytes::make_binary(to_string(res));
    }

    wrtc::SsrcGroup NegotiateChannelsMessage::deserializeSourceGroup(const json &ssrcGroup) {
        wrtc::SsrcGroup result;
        if (!ssrcGroup.contains("semantics") || !ssrcGroup.contains("ssrcs")) {
            throw ntgcalls::InvalidParams("Signaling: ssrcGroup must contain semantics and ssrcs");
        }
        result.semantics = ssrcGroup["semantics"];
        for (const auto &ssrc : ssrcGroup["ssrcs"].items()) {
            if (ssrc.value().is_string()) {
                uint32_t parsedSsrc = stringToUInt32(ssrc.value());
                if (parsedSsrc == 0) {
                    throw ntgcalls::InvalidParams("Signaling: parsedSsrc must not be 0");
                }
                result.ssrcs.push_back(parsedSsrc);
            } else if (ssrc.value().is_number()) {
                result.ssrcs.push_back(ssrc.value());
            } else {
                throw ntgcalls::InvalidParams("Signaling: ssrcs item must be a string or a number");
            }
        }
        return result;
    }

    wrtc::FeedbackType NegotiateChannelsMessage::deserializeFeedbackType(const json &feedbackType) {
        wrtc::FeedbackType result;
        if (!feedbackType.contains("type") || !feedbackType.contains("subtype")) {
            throw ntgcalls::InvalidParams("Signaling: feedbackType must contain type and subtype");
        }
        result.type = feedbackType["type"];
        result.subtype = feedbackType["subtype"];
        return result;
    }

    wrtc::PayloadType NegotiateChannelsMessage::deserializePayloadType(const json &payloadType) {
        wrtc::PayloadType result;
        if (!payloadType.contains("id") || !payloadType.contains("name") || !payloadType.contains("clockrate")) {
            throw ntgcalls::InvalidParams("Signaling: payloadType must contain id, name and clockrate");
        }
        result.id = payloadType["id"];
        result.name = payloadType["name"];
        result.clockrate = payloadType["clockrate"];
        if (payloadType.contains("channels")) {
            if (!payloadType["channels"].is_number()) {
                throw ntgcalls::InvalidParams("Signaling: channels must be a number");
            }
            result.channels = payloadType["channels"];
        }
        if (payloadType.contains("feedbackTypes")) {
            for (const auto &feedbackType : payloadType["feedbackTypes"].items()) {
                if (!feedbackType.value().is_object()) {
                    throw ntgcalls::InvalidParams("Signaling: feedbackTypes items must be objects");
                }
                result.feedbackTypes.push_back(deserializeFeedbackType(feedbackType.value()));
            }
        }
        if (payloadType.contains("parameters")) {
            for (const auto &parameter : payloadType["parameters"].items()) {
                if (!parameter.value().is_string()) {
                    throw ntgcalls::InvalidParams("Signaling: parameters items must be strings");
                }
                result.parameters.emplace_back(parameter.key(), parameter.value());
            }
        }
        return result;
    }

    webrtc::RtpExtension NegotiateChannelsMessage::deserializeRtpExtension(const json &rtpExtension) {
        webrtc::RtpExtension result;
        if (!rtpExtension.contains("id") || !rtpExtension.contains("uri")) {
            throw ntgcalls::InvalidParams("Signaling: rtpExtension must contain id and uri");
        }
        result.id = rtpExtension["id"];
        result.uri = rtpExtension["uri"];
        return result;
    }

    wrtc::MediaContent NegotiateChannelsMessage::deserializeContent(const json &content) {
        wrtc::MediaContent result;
        if (!content.contains("type") || !content.contains("ssrc")) {
            throw ntgcalls::InvalidParams("Signaling: content must contain type and ssrc");
        }
        if (const auto& type = content["type"]; type == "audio") {
            result.type = wrtc::MediaContent::Type::Audio;
        } else if (type == "video") {
            result.type = wrtc::MediaContent::Type::Video;
        } else {
            throw ntgcalls::InvalidParams("Signaling: type must be 'audio' or 'video'");
        }

        if (const auto& ssrc = content["ssrc"]; ssrc.is_string()) {
            result.ssrc = stringToUInt32(ssrc);
        } else if (ssrc.is_number()) {
            result.ssrc = static_cast<uint32_t>(ssrc);
        } else {
            throw ntgcalls::InvalidParams("Signaling: ssrc must be a string or a number");
        }
        if (content.contains("ssrcGroups")) {
            for (const auto &ssrcGroup : content["ssrcGroups"].items()) {
                if (!ssrcGroup.value().is_object()) {
                    throw ntgcalls::InvalidParams("Signaling: ssrcsGroups items must be objects");
                }
                result.ssrcGroups.push_back(deserializeSourceGroup(ssrcGroup.value()));
            }
        }
        if (content.contains("payloadTypes")) {
            for (const auto &payloadType : content["payloadTypes"].items()) {
                if (!payloadType.value().is_object()) {
                    throw ntgcalls::InvalidParams("Signaling: payloadTypes items must be objects");
                }
                result.payloadTypes.push_back(deserializePayloadType(payloadType.value()));
            }
        }
        if (content.contains("rtpExtensions")) {
            for (const auto &rtpExtension : content["rtpExtensions"].items()) {
                if (!rtpExtension.value().is_object()) {
                    throw ntgcalls::InvalidParams("Signaling: rtpExtensions items must be objects");
                }
                result.rtpExtensions.push_back(deserializeRtpExtension(rtpExtension.value()));
            }
        }
        return result;
    }

    std::unique_ptr<NegotiateChannelsMessage> NegotiateChannelsMessage::deserialize(const bytes::binary &data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<NegotiateChannelsMessage>();
        if (!j.contains("exchangeId")) {
            throw ntgcalls::InvalidParams("Signaling: exchangeId must be present");
        }
        if (const auto exchangeId = j["exchangeId"]; exchangeId.is_string()) {
            message->exchangeId = stringToUInt32(exchangeId);
        } else if (exchangeId.is_number()) {
            message->exchangeId = static_cast<uint32_t>(exchangeId);
        } else {
            throw ntgcalls::InvalidParams("Signaling: exchangeId must be a string or a number");
        }
        if (!j.contains("contents")) {
            throw ntgcalls::InvalidParams("Signaling: contents must be present");
        }
        for (const auto &content : j["contents"].items()) {
            if (!content.value().is_object()) {
                throw ntgcalls::InvalidParams("Signaling: contents items must be objects");
            }
            message->contents.push_back(deserializeContent(content.value()));
        }
        return std::move(message);
    }
} // signaling