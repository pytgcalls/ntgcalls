//
// Created by Lauren on 18/08/23.
//

#ifndef IS_ANDROID

#include <api/environment/environment_factory.h>
#include <modules/video_coding/codecs/vp8/include/vp8.h>
#include <modules/video_coding/codecs/vp9/include/vp9.h>
#include <wrtc/video_factory/software/google/google.hpp>

namespace google {

    void add_encoders(std::vector<wrtc::video_factory::VideoEncoderConfig> &encoders) {
        encoders.emplace_back(
          webrtc::kVideoCodecVP8,
          [](const auto&) {
              return CreateVp8Encoder(webrtc::CreateEnvironment());
          }
        );
        encoders.emplace_back(
            webrtc::kVideoCodecVP9,
            [](const auto&) {
                return CreateVp9Encoder(webrtc::CreateEnvironment());
            }
        );
    }

    void add_decoders(std::vector<wrtc::video_factory::VideoDecoderConfig> &decoders) {
        decoders.emplace_back(
           webrtc::kVideoCodecVP8,
           [](const auto&) {
               return CreateVp8Decoder(webrtc::CreateEnvironment());
           }
        );
        decoders.emplace_back(
            webrtc::kVideoCodecVP9,
            [](const auto&) {
                return webrtc::VP9Decoder::Create();
            }
        );
    }

} // google

#endif