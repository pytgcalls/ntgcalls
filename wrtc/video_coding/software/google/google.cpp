//
// Created by Laky64 on 18/08/2023.
//

#include "google.hpp"

namespace google {

    void addEncoders(std::shared_ptr<wrtc::VideoFactoryConfig> config) {
        config->encoders.push_back(
                wrtc::VideoEncoderConfig(
                        webrtc::kVideoCodecVP8,
                        [](auto format) {
                            return webrtc::VP8Encoder::Create();
                        }
                )
        );
        config->encoders.push_back(
                wrtc::VideoEncoderConfig(
                        webrtc::kVideoCodecVP9,
                        [](auto format) {
                            return webrtc::VP8Encoder::Create();
                        }
                )
        );
    }

    void addDecoders(std::shared_ptr<wrtc::VideoFactoryConfig> config) {
        config->decoders.push_back(
                wrtc::VideoDecoderConfig(
                        webrtc::kVideoCodecVP8,
                        [](auto format) {
                            return webrtc::VP8Decoder::Create();
                        }
                )
        );
        config->decoders.push_back(
                wrtc::VideoDecoderConfig(
                        webrtc::kVideoCodecVP9,
                        [](auto format) {
                            return webrtc::VP9Decoder::Create();
                        }
                )
        );
    }

} // libwebrtc