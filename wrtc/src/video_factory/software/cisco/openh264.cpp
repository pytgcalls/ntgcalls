//
// Created by Laky64 on 04/11/24.
//

#ifndef IS_ANDROID
#include <wrtc/video_factory/software/openh264/openh264.hpp>
#include <wrtc/video_factory/software/openh264/h264_encoder.hpp>
#include <wrtc/video_factory/software/openh264/h264_decoder.hpp>
#include <api/environment/environment_factory.h>

namespace openh264 {

    void addEncoders(std::vector<wrtc::VideoEncoderConfig>& encoders) {
        encoders.emplace_back(
            webrtc::kVideoCodecH264,
            [](auto) {
                return std::make_unique<H264Encoder>(webrtc::CreateEnvironment());
            }
        );
    }

    void addDecoders(std::vector<wrtc::VideoDecoderConfig>& decoders) {
        decoders.emplace_back(
            webrtc::kVideoCodecH264,
            [](auto) {
                return std::make_unique<H264Decoder>();
            }
        );
    }
} // openh264

#endif