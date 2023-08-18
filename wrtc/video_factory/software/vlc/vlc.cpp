//
// Created by Laky64 on 18/08/2023.
//

#include "vlc.hpp"

namespace vlc {

    void addEncoders(std::shared_ptr<wrtc::VideoFactoryConfig> config) {
#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
        config->encoders.push_back(
                wrtc::VideoEncoderConfig(
                        webrtc::kVideoCodecAV1,
                        [](auto format) {
                            return webrtc::CreateLibaomAv1Encoder();
                        }
                )
        );
#endif
    }

    void addDecoders(std::shared_ptr<wrtc::VideoFactoryConfig> config) {
#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
        config->decoders.push_back(
                wrtc::VideoDecoderConfig(
                        webrtc::kVideoCodecAV1,
                        [](auto format) {
                            return webrtc::CreateDav1dDecoder();
                        }
                )
        );
#endif
    }

} // vlc