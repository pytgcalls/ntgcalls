//
// Created by Lauren on 02/10/24.
//

#include <api/candidate.h>
#include <rtc_base/logging.h>
#include <rtc_base/socket_address.h>
#include <rtc_base/ssl_fingerprint.h>
#include <wrtc/exceptions.hpp>
#include <wrtc/models/peer_ice_parameters.hpp>
#include <wrtc/interfaces/response_payload.hpp>

namespace wrtc::interfaces {
    ResponsePayload::ResponsePayload(const std::string& payload) {
        utils::json data;
        try {
            data = utils::json::parse(payload);
        } catch (std::exception& e) {
            RTC_LOG(LS_ERROR) << "Invalid JSON: " << e.what();
            throw TransportParseException("Invalid JSON");
        }
        try {
            if (!data["rtmp"].is_null()) {
                is_rtmp = true;
                return;
            }
            if (!data["stream"].is_null()) {
                is_stream = true;
                return;
            }
            if (data["transport"].is_null()) {
                RTC_LOG(LS_ERROR) << "Transport not found";
                throw TransportParseException("Transport not found");
            }
            auto transport = data["transport"];
            remote_ice_parameters.ufrag = transport["ufrag"];
            remote_ice_parameters.pwd = transport["pwd"];
            for (const auto& candidate : transport["candidates"]) {
                webrtc::SocketAddress address(candidate["ip"].get<std::string>(), std::stoi(candidate["port"].get<std::string>()));
                webrtc::IceCandidateType candidate_type;
                if (auto raw_candidate_type = candidate["type"].get<std::string>(); raw_candidate_type == "srflx") {
                    candidate_type = webrtc::IceCandidateType::kSrflx;
                } else if (raw_candidate_type == "prflx") {
                    candidate_type = webrtc::IceCandidateType::kPrflx;
                } else if (raw_candidate_type == "relay") {
                    candidate_type = webrtc::IceCandidateType::kRelay;
                } else if (raw_candidate_type == "local" || raw_candidate_type == "host") {
                    candidate_type = webrtc::IceCandidateType::kHost;
                } else {
                    RTC_LOG(LS_ERROR) << "Invalid candidate type";
                    throw TransportParseException("Invalid candidate type");
                }
                candidates.emplace_back(
                    std::stoi(candidate["generation"].get<std::string>()),
                    candidate["protocol"].get<std::string>(),
                    address,
                    std::stoi(candidate["priority"].get<std::string>()),
                    remote_ice_parameters.ufrag,
                    remote_ice_parameters.pwd,
                    candidate_type,
                    static_cast<uint32_t>(std::stoi(candidate["generation"].get<std::string>())),
                    candidate["foundation"].get<std::string>(),
                    static_cast<uint16_t>(std::stoi(candidate["network"].get<std::string>())),
                    0
                );
            }
            if (!transport["fingerprints"].empty()) {
                fingerprint = webrtc::SSLFingerprint::CreateUniqueFromRfc4572(
                    transport["fingerprints"][0]["hash"].get<std::string>(),
                    transport["fingerprints"][0]["fingerprint"].get<std::string>()
                );
            }
            if (auto audio = data["audio"]; !audio.is_null()) {
                media.audio_payload_types = parse_payload_types(audio);
                media.audio_rtp_extensions = parse_rtp_extensions(audio);
            }
            auto video = data["video"];
            media.video_payload_types = parse_payload_types(video);
            media.video_rtp_extensions = parse_rtp_extensions(video);
        } catch (utils::json::exception& e) {
            RTC_LOG(LS_ERROR) << "Invalid JSON: " << e.what();
            throw TransportParseException("Invalid JSON: " + std::string(e.what()));
        }
    }

    std::vector<webrtc::RtpExtension> ResponsePayload::parse_rtp_extensions(const utils::json& data) {
        std::vector<webrtc::RtpExtension> result;
        for (const auto& extension : data["rtp-hdrexts"]) {
            webrtc::RtpExtension rtp_extension;
            rtp_extension.id = webrtc::RtpHeaderExtensionId(extension["id"].get<int>());
            rtp_extension.uri = extension["uri"];
            result.push_back(rtp_extension);
        }
        return result;
    }

    std::vector<models::PayloadType> ResponsePayload::parse_payload_types(const utils::json& data) {
        std::vector<models::PayloadType> result;
        for (const auto& payload : data["payload-types"]) {
            models::PayloadType payload_type;
            payload_type.id = payload["id"];
            payload_type.name = payload["name"];
            payload_type.clockrate = payload["clockrate"];
            if (!payload["channels"].is_null()) {
                payload_type.channels = payload["channels"];
            }
            if (!payload["parameters"].is_null()) {
                for (const auto& parameter : payload["parameters"].items()) {
                    std::string value;
                    if (parameter.value().is_string()) {
                        value = parameter.value();
                    } else {
                        value = std::to_string(parameter.value().get<int>());
                    }
                    payload_type.parameters.emplace_back(parameter.key(), value);
                }
            }
            if (!payload["rtcp-fbs"].is_null()) {
                for (const auto& feedback : payload["rtcp-fbs"]) {
                    std::string sub_type;
                    if (!feedback["subtype"].is_null()) {
                        sub_type = feedback["subtype"];
                    }
                    payload_type.feedback_types.push_back({feedback["type"], sub_type});
                }
            }
            result.push_back(payload_type);
        }
        return result;
    }
} // wrtc::interfaces