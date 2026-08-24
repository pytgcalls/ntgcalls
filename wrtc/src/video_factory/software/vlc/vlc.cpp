//
// Created by Lauren on 18/08/23.
//

#ifndef IS_ANDROID
#include <api/environment/environment_factory.h>
#include <wrtc/video_factory/software/vlc/vlc.hpp>

#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
#include <modules/video_coding/codecs/av1/dav1d_decoder.h>
#include <modules/video_coding/codecs/av1/libaom_av1_encoder.h>
#endif

namespace vlc {

    void add_encoders(std::vector<wrtc::video_factory::VideoEncoderConfig>& encoders) {
#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
        encoders.emplace_back(
            webrtc::kVideoCodecAV1,
            [](const auto&) {
                return CreateLibaomAv1Encoder(webrtc::CreateEnvironment());
            }
        );
#endif
    }

    void add_decoders(std::vector<wrtc::video_factory::VideoDecoderConfig>& decoders) {
#if !defined(__arm__) || defined(__aarch64__) || defined(__ARM_NEON__)
        decoders.emplace_back(
            webrtc::kVideoCodecAV1,
            [](const auto&) {
                return webrtc::CreateDav1dDecoder();
            }
        );
#endif
    }

} // vlc

#endif
