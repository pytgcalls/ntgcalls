//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include <vector>
#include "video_encoder_config.hpp"
#include "video_decoder_config.hpp"

namespace wrtc {

    struct VideoFactoryConfig {
        std::vector<VideoEncoderConfig> encoders;
        std::vector<VideoDecoderConfig> decoders;
    };

    struct VideoCoding {
        std::unique_ptr<webrtc::VideoEncoder> encoder;
        std::unique_ptr<webrtc::VideoDecoder> decoder;
    };

} // wrtc
