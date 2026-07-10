//
// Created by Lauren on 18/08/23.
//

#pragma once

#ifndef IS_ANDROID

#include <wrtc/video_factory/video_decoder_config.hpp>
#include <wrtc/video_factory/video_encoder_config.hpp>

namespace google {

    void add_encoders(std::vector<wrtc::video_factory::VideoEncoderConfig> &encoders);

    void add_decoders(std::vector<wrtc::video_factory::VideoDecoderConfig> &decoders);

} // google

#endif