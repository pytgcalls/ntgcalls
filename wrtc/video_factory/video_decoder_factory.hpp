//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include "base_video_factory.hpp"

namespace wrtc {

    class VideoDecoderFactory: public webrtc::VideoDecoderFactory, BaseVideoFactory {
    public:
        VideoDecoderFactory(std::vector<VideoDecoderConfig> decoders): decoders(decoders){};

    private:
        std::vector<VideoDecoderConfig> decoders;

        std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(const webrtc::SdpVideoFormat &format) override;

        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
    };

} // wrtc
