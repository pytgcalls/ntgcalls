//
// Created by Lauren on 04/11/24.
//

#ifndef IS_ANDROID
#include <api/environment/environment_factory.h>
#include <wrtc/video_factory/software/openh264/h264_decoder.hpp>
#include <wrtc/video_factory/software/openh264/h264_encoder.hpp>
#include <wrtc/video_factory/software/openh264/openh264.hpp>

namespace openh264 {

    void add_encoders(std::vector<wrtc::video_factory::VideoEncoderConfig>& encoders) {
        encoders.emplace_back(
            webrtc::kVideoCodecH264,
            [](const auto&) {
                return std::make_unique<H264Encoder>(webrtc::CreateEnvironment());
            }
        );
    }

    void add_decoders(std::vector<wrtc::video_factory::VideoDecoderConfig>& decoders) {
        decoders.emplace_back(
            webrtc::kVideoCodecH264,
            [](const auto&) {
                return std::make_unique<H264Decoder>();
            }
        );
    }
} // openh264

#endif
