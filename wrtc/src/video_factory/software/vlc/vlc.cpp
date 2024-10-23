//
// Created by Laky64 on 18/08/2023.
//

#ifndef IS_ANDROID
#include <wrtc/video_factory/software/vlc/vlc.hpp>
#include <api/environment/environment_factory.h>

#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
#include <modules/video_coding/codecs/av1/dav1d_decoder.h>
#include <modules/video_coding/codecs/av1/libaom_av1_encoder.h>
#endif

namespace vlc {

    void addEncoders(std::vector<wrtc::VideoEncoderConfig> &encoders) {
#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
        encoders.emplace_back(
            webrtc::kVideoCodecAV1,
            [](auto) {
                return CreateLibaomAv1Encoder(webrtc::CreateEnvironment());
            }
        );
#endif
    }

    void addDecoders(std::vector<wrtc::VideoDecoderConfig> &decoders) {
#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
        decoders.emplace_back(
            webrtc::kVideoCodecAV1,
            [](auto) {
                return webrtc::CreateDav1dDecoder();
            }
        );
#endif
    }

} // vlc

#endif