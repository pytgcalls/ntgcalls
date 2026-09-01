//
// Created by Lauren on 30/03/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/signaling/messages/negotiate_channels_message.hpp>

namespace ntgcalls::signaling::messages {
    json NegotiateChannelsMessage::serialize_source_group(const wrtc::models::SsrcGroup& ssrc_group) {
        auto ssrcs_json = json::array();
        for (const auto ssrc : ssrc_group.ssrcs) {
            ssrcs_json.push_back(std::to_string(ssrc));
        }
        return {
            {"semantics", ssrc_group.semantics},
            {"ssrcs", ssrcs_json},
        };
    }

    json NegotiateChannelsMessage::serialize_payload_type(const wrtc::models::PayloadType& payload_type) {
        json res{
            {"id", payload_type.id},
            {"name", payload_type.name},
            {"clockrate", payload_type.clockrate},
            {"channels", payload_type.channels},
        };
        auto feedback_types_json = json::array();
        if (!payload_type.feedback_types.empty()) {
            for (const auto& [type, subtype] : payload_type.feedback_types) {
                feedback_types_json.push_back({
                    {"type", type},
                    {"subtype", subtype},
                });
            }
        }
        res["feedbackTypes"] = feedback_types_json;
        auto parameters_json = json::object();
        for (const auto& [key, value] : payload_type.parameters) {
            parameters_json[key] = value;
        }
        res["parameters"] = parameters_json;
        return res;
    }

    json NegotiateChannelsMessage::serialize_content(const wrtc::models::MediaContent& content) {
        json content_json{
            {"type", content.type == wrtc::models::MediaContent::Type::Audio ? "audio" : "video"},
            {"ssrc", std::to_string(content.ssrc)},
        };
        if (!content.ssrc_groups.empty()) {
            auto ssrc_groups_json = json::array();
            for (const auto& ssrc_group : content.ssrc_groups) {
                ssrc_groups_json.push_back(serialize_source_group(ssrc_group));
            }
            content_json["ssrcGroups"] = ssrc_groups_json;
        }
        if (!content.payload_types.empty()) {
            auto payload_types_json = json::array();
            for (const auto& payload_type : content.payload_types) {
                payload_types_json.push_back(serialize_payload_type(payload_type));
            }
            content_json["payloadTypes"] = payload_types_json;
        }
        auto rtp_extensions_json = json::array();
        for (const auto& rtp_extension : content.rtp_extensions) {
            rtp_extensions_json.push_back(json{
                {"uri", rtp_extension.uri},
                {"id", rtp_extension.id.value()},
            });
        }
        content_json["rtpExtensions"] = rtp_extensions_json;
        return content_json;
    }

    bytes::binary NegotiateChannelsMessage::serialize() const {
        json res{
            {"@type", "NegotiateChannels"},
            {"exchangeId", std::to_string(exchange_id)},
        };
        auto contents_json = json::array();
        for (const auto& content : contents) {
            contents_json.push_back(serialize_content(content));
        }
        res["contents"] = contents_json;
        return bytes::make_binary(res.dump());
    }

    wrtc::models::SsrcGroup NegotiateChannelsMessage::deserialize_source_group(const json& ssrc_group) {
        wrtc::models::SsrcGroup result;
        if (!ssrc_group.contains("semantics") || !ssrc_group.contains("ssrcs")) {
            throw InvalidParams("Signaling: ssrcGroup must contain semantics and ssrcs");
        }
        result.semantics = ssrc_group["semantics"];
        for (const auto& ssrc : ssrc_group["ssrcs"]) {
            if (ssrc.is_string()) {
                const uint32_t parsed_ssrc = string_to_uint32(ssrc);
                if (parsed_ssrc == 0) {
                    throw InvalidParams("Signaling: parsedSsrc must not be 0");
                }
                result.ssrcs.push_back(parsed_ssrc);
            } else if (ssrc.is_number()) {
                result.ssrcs.push_back(ssrc);
            } else {
                throw InvalidParams("Signaling: ssrcs item must be a string or a number");
            }
        }
        return result;
    }

    wrtc::models::FeedbackType NegotiateChannelsMessage::deserialize_feedback_type(const json& feedback_type) {
        wrtc::models::FeedbackType result;
        if (!feedback_type.contains("type") || !feedback_type.contains("subtype")) {
            throw InvalidParams("Signaling: feedbackType must contain type and subtype");
        }
        result.type = feedback_type["type"];
        result.subtype = feedback_type["subtype"];
        return result;
    }

    wrtc::models::PayloadType NegotiateChannelsMessage::deserialize_payload_type(const json& payload_type) {
        wrtc::models::PayloadType result;
        if (!payload_type.contains("id") || !payload_type.contains("name") || !payload_type.contains("clockrate")) {
            throw InvalidParams("Signaling: payloadType must contain id, name and clockrate");
        }
        result.id = payload_type["id"];
        result.name = payload_type["name"];
        result.clockrate = payload_type["clockrate"];
        if (payload_type.contains("channels")) {
            if (!payload_type["channels"].is_number()) {
                throw InvalidParams("Signaling: channels must be a number");
            }
            result.channels = payload_type["channels"];
        }
        if (payload_type.contains("feedbackTypes")) {
            for (const auto& feedback_type : payload_type["feedbackTypes"]) {
                if (!feedback_type.is_object()) {
                    throw InvalidParams("Signaling: feedbackTypes items must be objects");
                }
                result.feedback_types.push_back(deserialize_feedback_type(feedback_type));
            }
        }
        if (payload_type.contains("parameters")) {
            for (const auto& parameter : payload_type["parameters"].items()) {
                if (!parameter.value().is_string()) {
                    throw InvalidParams("Signaling: parameters items must be strings");
                }
                result.parameters.emplace_back(parameter.key(), parameter.value());
            }
        }
        return result;
    }

    webrtc::RtpExtension NegotiateChannelsMessage::deserialize_rtp_extension(const json& rtp_extension) {
        webrtc::RtpExtension result;
        if (!rtp_extension.contains("id") || !rtp_extension.contains("uri")) {
            throw InvalidParams("Signaling: rtpExtension must contain id and uri");
        }
        result.id = webrtc::RtpHeaderExtensionId(rtp_extension["id"].get<int>());
        result.uri = rtp_extension["uri"];
        return result;
    }

    wrtc::models::MediaContent NegotiateChannelsMessage::deserialize_content(const json& content) {
        wrtc::models::MediaContent result;
        if (!content.contains("type") || !content.contains("ssrc")) {
            throw InvalidParams("Signaling: content must contain type and ssrc");
        }
        if (const auto& type = content["type"]; type == "audio") {
            result.type = wrtc::models::MediaContent::Type::Audio;
        } else if (type == "video") {
            result.type = wrtc::models::MediaContent::Type::Video;
        } else {
            throw InvalidParams("Signaling: type must be 'audio' or 'video'");
        }

        if (const auto& ssrc = content["ssrc"]; ssrc.is_string()) {
            result.ssrc = string_to_uint32(ssrc);
        } else if (ssrc.is_number()) {
            result.ssrc = static_cast<uint32_t>(ssrc);
        } else {
            throw InvalidParams("Signaling: ssrc must be a string or a number");
        }
        if (content.contains("ssrcGroups")) {
            for (const auto& ssrc_group : content["ssrcGroups"]) {
                if (!ssrc_group.is_object()) {
                    throw InvalidParams("Signaling: ssrcsGroups items must be objects");
                }
                result.ssrc_groups.push_back(deserialize_source_group(ssrc_group));
            }
        }
        if (content.contains("payloadTypes")) {
            for (const auto& payload_type : content["payloadTypes"]) {
                if (!payload_type.is_object()) {
                    throw InvalidParams("Signaling: payloadTypes items must be objects");
                }
                result.payload_types.push_back(deserialize_payload_type(payload_type));
            }
        }
        if (content.contains("rtpExtensions")) {
            for (const auto& rtp_extension : content["rtpExtensions"]) {
                if (!rtp_extension.is_object()) {
                    throw InvalidParams("Signaling: rtpExtensions items must be objects");
                }
                result.rtp_extensions.push_back(deserialize_rtp_extension(rtp_extension));
            }
        }
        return result;
    }

    std::unique_ptr<NegotiateChannelsMessage> NegotiateChannelsMessage::deserialize(const bytes::binary& data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<NegotiateChannelsMessage>();
        if (!j.contains("exchangeId")) {
            throw InvalidParams("Signaling: exchangeId must be present");
        }
        if (const auto exchange_id = j["exchangeId"]; exchange_id.is_string()) {
            message->exchange_id = string_to_uint32(exchange_id);
        } else if (exchange_id.is_number()) {
            message->exchange_id = static_cast<uint32_t>(exchange_id);
        } else {
            throw InvalidParams("Signaling: exchangeId must be a string or a number");
        }
        if (!j.contains("contents")) {
            throw InvalidParams("Signaling: contents must be present");
        }
        for (const auto& content : j["contents"]) {
            if (!content.is_object()) {
                throw InvalidParams("Signaling: contents items must be objects");
            }
            message->contents.push_back(deserialize_content(content));
        }
        return std::move(message);
    }
} // ntgcalls::signaling::messages
