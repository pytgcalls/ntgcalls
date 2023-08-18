//
// Created by Laky64 on 18/08/2023.
//

#include "vlc.hpp"

namespace vlc {

    void addEncoders(std::vector<wrtc::VideoEncoderConfig> &encoders) {
#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
        encoders.push_back(
                wrtc::VideoEncoderConfig(
                        webrtc::kVideoCodecAV1,
                        [](auto format) {
                            return webrtc::CreateLibaomAv1Encoder();
                        }
                )
        );
#endif
    }

    void addDecoders(std::vector<wrtc::VideoDecoderConfig> &decoders) {
#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
        decoders.push_back(
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