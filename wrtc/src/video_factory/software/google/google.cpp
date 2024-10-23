//
// Created by Laky64 on 18/08/2023.
//

#ifndef IS_ANDROID

#include <wrtc/video_factory/software/google/google.hpp>

#include <api/environment/environment_factory.h>
#include <modules/video_coding/codecs/vp8/include/vp8.h>
#include <modules/video_coding/codecs/vp9/include/vp9.h>

namespace google {

    void addEncoders(std::vector<wrtc::VideoEncoderConfig> &encoders) {
        encoders.emplace_back(
          webrtc::kVideoCodecVP8,
          [](auto) {
              return CreateVp8Encoder(webrtc::CreateEnvironment());
          }
        );
        encoders.emplace_back(
            webrtc::kVideoCodecVP9,
            [](auto) {
                return CreateVp9Encoder(webrtc::CreateEnvironment());
            }
        );
    }

    void addDecoders(std::vector<wrtc::VideoDecoderConfig> &decoders) {
        decoders.emplace_back(
           webrtc::kVideoCodecVP8,
           [](auto) {
               return CreateVp8Decoder(webrtc::CreateEnvironment());
           }
        );
        decoders.emplace_back(
            webrtc::kVideoCodecVP9,
            [](auto) {
                return webrtc::VP9Decoder::Create();
            }
        );
    }

} // google

#endif