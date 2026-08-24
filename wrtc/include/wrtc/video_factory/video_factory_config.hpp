//
// Created by Lauren on 18/08/23.
//

#pragma once

#ifndef IS_ANDROID
#include <vector>
#include <wrtc/video_factory/video_decoder_factory.hpp>
#include <wrtc/video_factory/video_encoder_factory.hpp>

namespace wrtc::video_factory {

    class VideoFactoryConfig {
    public:
        std::vector<VideoEncoderConfig> encoders;
        std::vector<VideoDecoderConfig> decoders;

        explicit VideoFactoryConfig();

        std::unique_ptr<VideoEncoderFactory> CreateVideoEncoderFactory();

        std::unique_ptr<VideoDecoderFactory> CreateVideoDecoderFactory();
    };

} // wrtc::video_factory

#endif
