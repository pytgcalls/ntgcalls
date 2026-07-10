//
// Created by Lauren on 18/08/23.
//

#pragma once

#include <wrtc/video_factory/video_decoder_config.hpp>

namespace wrtc::video_factory {

    class VideoDecoderFactory final : public webrtc::VideoDecoderFactory {
        std::vector<VideoDecoderConfig> decoders_;
        mutable std::vector<std::vector<webrtc::SdpVideoFormat>> formats_;

    public:
        explicit VideoDecoderFactory(const std::vector<VideoDecoderConfig>& decoders): decoders_(decoders){}

        std::unique_ptr<webrtc::VideoDecoder> Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat &format) override;

        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
    };

} // wrtc::video_factory
