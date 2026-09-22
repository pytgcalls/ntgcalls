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

        std::unique_ptr<VideoEncoderFactory> create_video_encoder_factory();

        std::unique_ptr<VideoDecoderFactory> create_video_decoder_factory();
    };

} // wrtc::video_factory

#endif
