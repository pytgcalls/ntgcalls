//
// Created by Laky64 on 04/11/24.
//

#pragma once
#include <vector>
#include <api/video_codecs/sdp_video_format.h>
#include <media/base/codec.h>

namespace wrtc {

    class OutgoingVideoFormat {
    public:
        cricket::Codec videoCodec;
        std::optional<cricket::Codec> rtxCodec;

        OutgoingVideoFormat(cricket::Codec videoCodec_, std::optional<cricket::Codec> rtxCodec_);

        static std::vector<OutgoingVideoFormat> assignPayloadTypes(std::vector<webrtc::SdpVideoFormat> const &formats);

        static void addDefaultFeedbackParams(cricket::Codec *codec);
    };

} // wrtc
