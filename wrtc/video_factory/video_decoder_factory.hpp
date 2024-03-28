//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include "video_decoder_config.hpp"

namespace wrtc {

    class VideoDecoderFactory final : public webrtc::VideoDecoderFactory {
    public:
        explicit VideoDecoderFactory(const std::vector<VideoDecoderConfig>& decoders): decoders(decoders){};

    private:
        std::vector<VideoDecoderConfig> decoders;
        mutable std::vector<std::vector<webrtc::SdpVideoFormat>> formats_;

        std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(const webrtc::SdpVideoFormat &format) override;

        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
    };

} // wrtc
