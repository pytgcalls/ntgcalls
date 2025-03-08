//
// Created by Laky64 on 04/11/24.
//

#pragma once
#include <vector>
#include <media/base/codec.h>
#include <wrtc/models/media_content.hpp>
#include <api/video_codecs/sdp_video_format.h>

namespace wrtc {
    struct PayloadType;

    class OutgoingVideoFormat {
        static std::vector<OutgoingVideoFormat> assignPayloadTypes(std::vector<webrtc::SdpVideoFormat> const &formats);

        static void addDefaultFeedbackParams(cricket::Codec *codec);

    public:
        cricket::Codec videoCodec;
        std::optional<cricket::Codec> rtxCodec;

        OutgoingVideoFormat(cricket::Codec videoCodec_, std::optional<cricket::Codec> rtxCodec_);

        static std::vector<cricket::Codec> getVideoCodecs(
            const std::vector<webrtc::SdpVideoFormat>& formats,
            const std::vector<PayloadType>& payloadTypes,
            bool isGroupConnection
        );
    };

} // wrtc
