//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include <modules/video_coding/codecs/h264/include/h264.h>

#include "../../video_encoder_config.hpp"
#include "../../video_decoder_config.hpp"

namespace google {

    void addEncoders(std::vector<wrtc::VideoEncoderConfig> &encoders);

    void addDecoders(std::vector<wrtc::VideoDecoderConfig> &decoders);

} // google
