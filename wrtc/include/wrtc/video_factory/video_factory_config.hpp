//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include <vector>
#include "video_encoder_factory.hpp"
#include "video_decoder_factory.hpp"
#include "software/google/google.hpp"


namespace wrtc {

    class VideoFactoryConfig {
    public:
        std::vector<VideoEncoderConfig> encoders;
        std::vector<VideoDecoderConfig> decoders;

        VideoFactoryConfig();

        std::unique_ptr<VideoEncoderFactory> CreateVideoEncoderFactory();

        std::unique_ptr<VideoDecoderFactory> CreateVideoDecoderFactory();
    };

} // wrtc
