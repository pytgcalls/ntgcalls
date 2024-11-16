//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#ifndef IS_ANDROID
#include <vector>
#include <wrtc/video_factory/video_encoder_factory.hpp>
#include <wrtc/video_factory/video_decoder_factory.hpp>

namespace wrtc {

    class VideoFactoryConfig {
        static bool allowH264Encoder;
    public:
        std::vector<VideoEncoderConfig> encoders;
        std::vector<VideoDecoderConfig> decoders;

        explicit VideoFactoryConfig();

        std::unique_ptr<VideoEncoderFactory> CreateVideoEncoderFactory();

        std::unique_ptr<VideoDecoderFactory> CreateVideoDecoderFactory();

        static void EnableH264Encoder(bool enable);
    };

} // wrtc

#endif