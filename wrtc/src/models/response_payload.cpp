//
// Created by Laky64 on 02/10/24.
//

#include <api/candidate.h>
#include <rtc_base/logging.h>
#include <rtc_base/socket_address.h>
#include <rtc_base/ssl_fingerprint.h>
#include <wrtc/exceptions.hpp>
#include <wrtc/models/peer_ice_parameters.hpp>
#include <wrtc/models/response_payload.hpp>

namespace wrtc {
    ResponsePayload::ResponsePayload(const std::string& payload) {
        json data;
        try {
            data = json::parse(payload);
        } catch (std::exception& e) {
            RTC_LOG(LS_ERROR) << "Invalid JSON: " << e.what();
            throw TransportParseException("Invalid JSON");
        }
        try {
            if (!data["rtmp"].is_null()) {
                isRtmp = true;
                return;
            }
            if (data["transport"].is_null()) {
                RTC_LOG(LS_ERROR) << "Transport not found";
                throw TransportParseException("Transport not found");
            }
            auto transport = data["transport"];
            remoteIceParameters.ufrag = transport["ufrag"].get<std::string>();
            remoteIceParameters.pwd = transport["pwd"].get<std::string>();
            for (const auto& item : transport["candidates"].items()) {
                auto candidate = item.value();
                rtc::SocketAddress address(candidate["ip"].get<std::string>(), std::stoi(candidate["port"].get<std::string>()));
                webrtc::IceCandidateType candidateType;
                if (auto rawCandidateType = candidate["type"].get<std::string>(); rawCandidateType == "srflx") {
                    candidateType = webrtc::IceCandidateType::kSrflx;
                } else if (rawCandidateType == "prflx") {
                    candidateType = webrtc::IceCandidateType::kPrflx;
                } else if (rawCandidateType == "relay") {
                    candidateType = webrtc::IceCandidateType::kRelay;
                } else if (rawCandidateType == "local" || rawCandidateType == "host") {
                    candidateType = webrtc::IceCandidateType::kHost;
                } else {
                    RTC_LOG(LS_ERROR) << "Invalid candidate type";
                    throw TransportParseException("Invalid candidate type");
                }
                candidates.emplace_back(
                    std::stoi(candidate["generation"].get<std::string>()),
                    candidate["protocol"].get<std::string>(),
                    address,
                    std::stoi(candidate["priority"].get<std::string>()),
                    remoteIceParameters.ufrag,
                    remoteIceParameters.pwd,
                    candidateType,
                    static_cast<uint32_t>(std::stoi(candidate["generation"].get<std::string>())),
                    candidate["foundation"].get<std::string>(),
                    static_cast<uint16_t>(std::stoi(candidate["network"].get<std::string>())),
                    0
                );
            }
            if (!transport["fingerprints"].empty()) {
                fingerprint = rtc::SSLFingerprint::CreateUniqueFromRfc4572(
                    transport["fingerprints"][0]["hash"].get<std::string>(),
                    transport["fingerprints"][0]["fingerprint"].get<std::string>()
                );
            }
            if (auto audio = data["audio"]; !audio.is_null()) {
                media.audioPayloadTypes = parsePayloadTypes(audio);
                media.audioRtpExtensions = parseRtpExtensions(audio);
            }
            auto video = data["video"];
            media.videoPayloadTypes = parsePayloadTypes(video);
            media.videoRtpExtensions = parseRtpExtensions(video);
        } catch (json::exception& e) {
            RTC_LOG(LS_ERROR) << "Invalid JSON: " << e.what();
            throw TransportParseException("Invalid JSON: " + std::string(e.what()));
        }
    }

    std::vector<webrtc::RtpExtension> ResponsePayload::parseRtpExtensions(const json& data) {
        std::vector<webrtc::RtpExtension> result;
        for (const auto& item : data["rtp-hdrexts"].items()) {
            auto extension = item.value();
            webrtc::RtpExtension rtpExtension;
            rtpExtension.id =extension["id"].get<int>();
            rtpExtension.uri = extension["uri"].get<std::string>();
            result.push_back(rtpExtension);
        }
        return result;
    }

    std::vector<PayloadType> ResponsePayload::parsePayloadTypes(const json& data) {
        std::vector<PayloadType> result;
        for (const auto& item : data["payload-types"].items()) {
            auto payload = item.value();
            PayloadType payloadType;
            payloadType.id = payload["id"].get<int>();
            payloadType.name = payload["name"].get<std::string>();
            payloadType.clockrate = payload["clockrate"].get<int>();
            if (!payload["channels"].is_null()) {
                payloadType.channels = payload["channels"].get<int>();
            }
            if (!payload["parameters"].is_null()) {
                for (const auto& parameter : payload["parameters"].items()) {
                    std::string value;
                    if (parameter.value().is_string()) {
                        value = parameter.value().get<std::string>();
                    } else {
                        value = std::to_string(parameter.value().get<int>());
                    }
                    payloadType.parameters.emplace_back(parameter.key(), value);
                }
            }
            if (!payload["rtcp-fbs"].is_null()) {
                for (const auto& feedbackType : payload["rtcp-fbs"].items()) {
                    auto feedback = feedbackType.value();
                    std::string subType;
                    if (!feedback["subtype"].is_null()) {
                        subType = feedback["subtype"].get<std::string>();
                    }
                    payloadType.feedbackTypes.push_back({feedback["type"].get<std::string>(), subType});
                }
            }
            result.push_back(payloadType);
        }
        return result;
    }
} // wrtc