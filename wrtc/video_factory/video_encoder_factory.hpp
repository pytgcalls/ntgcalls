//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include <api/video_codecs/video_encoder_factory.h>

#include "video_encoder_config.hpp"

namespace wrtc {

    class VideoEncoderFactory: public webrtc::VideoEncoderFactory {
    public:
        VideoEncoderFactory(std::vector<VideoEncoderConfig> encoders): encoders(encoders){};

    private:
        std::vector<VideoEncoderConfig> encoders;
        mutable std::vector<std::vector<webrtc::SdpVideoFormat>> formats_;

        std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(const webrtc::SdpVideoFormat &format) override;

        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
    };

} // wrtc
