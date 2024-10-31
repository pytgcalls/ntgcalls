//
// Created by Laky64 on 02/10/24.
//

#pragma once
#include <nlohmann/json.hpp>
#include <wrtc/models/media_content.hpp>

namespace wrtc {
    using nlohmann::json;

    class ResponsePayload {
        static std::vector<webrtc::RtpExtension> parseRtpExtensions(const json& data);

        static std::vector<PayloadType> parsePayloadTypes(const json& data);

    public:
        struct Media {
            std::vector<PayloadType> audioPayloadTypes, videoPayloadTypes;
            std::vector<webrtc::RtpExtension> audioRtpExtensions, videoRtpExtensions;
        };

        PeerIceParameters remoteIceParameters;
        std::unique_ptr<rtc::SSLFingerprint> fingerprint;
        std::vector<cricket::Candidate> candidates;
        Media media;

        bool isRtmp = false;

        explicit ResponsePayload(const std::string& payload);
    };

} // wrtc
