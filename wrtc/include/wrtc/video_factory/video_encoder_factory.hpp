//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include <api/video_codecs/video_encoder_factory.h>
#include <wrtc/video_factory/video_encoder_config.hpp>

namespace wrtc {

    class VideoEncoderFactory final : public webrtc::VideoEncoderFactory {
    public:
        explicit VideoEncoderFactory(const std::vector<VideoEncoderConfig>& encoders): encoders(encoders){};

    private:
        std::vector<VideoEncoderConfig> encoders;
        mutable std::vector<std::vector<webrtc::SdpVideoFormat>> formats_;

        std::unique_ptr<webrtc::VideoEncoder> Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) override;

        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
    };

} // wrtc
