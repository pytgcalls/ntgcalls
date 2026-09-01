//
// Created by Lauren on 04/11/24.
//

#pragma once
#include <vector>
#include <api/video_codecs/sdp_video_format.h>
#include <media/base/codec.h>
#include <wrtc/models/media_content.hpp>

namespace wrtc::models {
    class OutgoingVideoFormat {
        webrtc::Codec video_codec_;
        std::optional<webrtc::Codec> rtx_codec_;

        static void add_default_feedback_params(webrtc::Codec* codec);

    public:
        OutgoingVideoFormat(webrtc::Codec video_codec, std::optional<webrtc::Codec> rtx_codec);

        static std::vector<webrtc::Codec> get_video_codecs(
            const std::vector<webrtc::SdpVideoFormat>& formats,
            const std::vector<PayloadType>& payload_types,
            bool is_group_connection
        );

        [[nodiscard]] webrtc::Codec video_codec() const;

        static std::vector<OutgoingVideoFormat> assign_payload_types(std::vector<webrtc::SdpVideoFormat> const& formats);
    };

} // wrtc::models
