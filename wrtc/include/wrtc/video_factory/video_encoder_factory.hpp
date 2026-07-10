//
// Created by Lauren on 18/08/23.
//

#pragma once

#include <api/video_codecs/video_encoder_factory.h>
#include <wrtc/video_factory/video_encoder_config.hpp>

namespace wrtc::video_factory {

    class VideoEncoderFactory final : public webrtc::VideoEncoderFactory {
        std::vector<VideoEncoderConfig> encoders_;
        mutable std::vector<std::vector<webrtc::SdpVideoFormat>> formats_;

    public:
        explicit VideoEncoderFactory(const std::vector<VideoEncoderConfig>& encoders): encoders_(encoders){};

        std::unique_ptr<webrtc::VideoEncoder> Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) override;

        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
    };

} // wrtc::video_factory
