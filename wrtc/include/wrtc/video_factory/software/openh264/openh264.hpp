//
// Created by Laky64 on 04/11/24.
//

#pragma once

#ifndef IS_ANDROID
#include <wrtc/video_factory/video_encoder_config.hpp>
#include <wrtc/video_factory/video_decoder_config.hpp>

namespace openh264 {

    void addEncoders(std::vector<wrtc::VideoEncoderConfig> &encoders);

    void addDecoders(std::vector<wrtc::VideoDecoderConfig> &decoders);

} // openh264

#endif