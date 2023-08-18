//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include "base_video_factory.hpp"

namespace wrtc {

    class VideoEncoderFactory: public webrtc::VideoEncoderFactory, BaseVideoFactory {
    public:
        VideoEncoderFactory(std::vector<VideoEncoderConfig> encoders): encoders(encoders){};

    private:
        std::vector<VideoEncoderConfig> encoders;

        std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(const webrtc::SdpVideoFormat &format) override;

        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
    };

} // wrtc
