//
// Created by Laky64 on 18/08/2023.
//

#include "google.hpp"

#include <modules/video_coding/codecs/vp8/include/vp8.h>
#include <modules/video_coding/codecs/vp9/include/vp9.h>

namespace google {

    void addEncoders(std::vector<wrtc::VideoEncoderConfig> &encoders) {
        encoders.emplace_back(
          webrtc::kVideoCodecVP8,
          [](auto) {
              return webrtc::VP8Encoder::Create();
          }
        );
        encoders.emplace_back(
            webrtc::kVideoCodecVP9,
            [](auto) {
                return webrtc::VP8Encoder::Create();
            }
        );
    }

    void addDecoders(std::vector<wrtc::VideoDecoderConfig> &decoders) {
        decoders.emplace_back(
           webrtc::kVideoCodecVP8,
           [](auto) {
               return webrtc::VP8Decoder::Create();
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