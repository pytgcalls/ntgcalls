//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include <memory>

#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
#include <modules/video_coding/codecs/av1/dav1d_decoder.h>
#include <modules/video_coding/codecs/av1/libaom_av1_encoder.h>
#endif

#include "../../video_encoder_config.hpp"
#include "../../video_decoder_config.hpp"

namespace vlc {

    void addEncoders(std::vector<wrtc::VideoEncoderConfig> &encoders);

    void addDecoders(std::vector<wrtc::VideoDecoderConfig> &decoders);

} // vlc
