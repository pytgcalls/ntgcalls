//
// Created by Laky64 on 18/08/2023.
//

#include "google.hpp"

namespace google {

    void addEncoders(std::vector<wrtc::VideoEncoderConfig> &encoders) {
        encoders.push_back(
                wrtc::VideoEncoderConfig(
                        webrtc::kVideoCodecVP8,
                        [](auto format) {
                            return webrtc::VP8Encoder::Create();
                        }
                )
        );
        encoders.push_back(
                wrtc::VideoEncoderConfig(
                        webrtc::kVideoCodecVP9,
                        [](auto format) {
                            return webrtc::VP8Encoder::Create();
                        }
                )
        );
    }

    void addDecoders(std::vector<wrtc::VideoDecoderConfig> &decoders) {
        decoders.push_back(
                wrtc::VideoDecoderConfig(
                        webrtc::kVideoCodecVP8,
                        [](auto format) {
                            return webrtc::VP8Decoder::Create();
                        }
                )
        );
        decoders.push_back(
                wrtc::VideoDecoderConfig(
                        webrtc::kVideoCodecVP9,
                        [](auto format) {
                            return webrtc::VP9Decoder::Create();
                        }
                )
        );
    }

} // google