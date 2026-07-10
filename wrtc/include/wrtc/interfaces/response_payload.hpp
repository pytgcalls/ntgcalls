//
// Created by Lauren on 02/10/24.
//

#pragma once
#include <wrtc/models/media_content.hpp>
#include <wrtc/utils/json.hpp>

namespace wrtc::interfaces {
    class ResponsePayload {
        static std::vector<webrtc::RtpExtension> parse_rtp_extensions(const utils::json& data);

        static std::vector<models::PayloadType> parse_payload_types(const utils::json& data);

    public:
        struct Media {
            std::vector<models::PayloadType> audio_payload_types, video_payload_types;
            std::vector<webrtc::RtpExtension> audio_rtp_extensions, video_rtp_extensions;
        };

        models::PeerIceParameters remote_ice_parameters;
        std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
        std::vector<webrtc::Candidate> candidates;
        Media media;

        bool is_rtmp = false;
        bool is_stream = false;

        explicit ResponsePayload(const std::string& payload);
    };

} // wrtc::interfaces
