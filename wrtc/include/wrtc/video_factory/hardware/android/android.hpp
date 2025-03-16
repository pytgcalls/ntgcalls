//
// Created by Laky64 on 15/09/24.
//

#pragma once

#include <wrtc/video_factory/video_encoder_config.hpp>
#include <wrtc/video_factory/video_decoder_config.hpp>

namespace android {

    void addEncoders(std::vector<wrtc::VideoEncoderConfig> &encoders, void* jniEnv);

    void addDecoders(std::vector<wrtc::VideoDecoderConfig> &decoders, void* jniEnv);

} // android
