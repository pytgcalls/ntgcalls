//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include <vector>
#include <api/video_codecs/sdp_video_format.h>

#include "video_encoder_config.hpp"
#include "video_decoder_config.hpp"

namespace wrtc {

    class BaseVideoFactory {
    private:
        mutable std::vector<std::vector<webrtc::SdpVideoFormat>> formats_;

    protected:
        template <typename Y>
        std::vector<webrtc::SdpVideoFormat> internalFormats(std::vector<Y> configs) const;

        template <typename X, typename Y>
        std::unique_ptr<X> internalVideo(std::vector<Y> input, const webrtc::SdpVideoFormat& format);
    };

} // wrtc
