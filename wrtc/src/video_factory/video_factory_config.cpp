//
// Created by Laky64 on 18/08/2023.
//

#ifndef IS_ANDROID
#include <wrtc/video_factory/video_factory_config.hpp>
#include <wrtc/video_factory/software/google/google.hpp>
#include <wrtc/video_factory/software/vlc/vlc.hpp>
#include <wrtc/video_factory/software/openh264/openh264.hpp>

namespace wrtc {

    bool VideoFactoryConfig::allowH264Encoder = true;

    VideoFactoryConfig::VideoFactoryConfig() {
        // Google (Software, VP9, VP8)
        google::addEncoders(encoders);
        google::addDecoders(decoders);

        // VLC (Software, AV1)
        vlc::addEncoders(encoders);
        vlc::addDecoders(decoders);

        // OpenH264 (Software, H264)
        if (allowH264Encoder) {
            openh264::addEncoders(encoders);
        }
        openh264::addDecoders(decoders);

        // NVCODEC (Hardware, VP8, VP9, H264)
        // TODO: @Laky-64 Add NVCODEC encoder-decoder when available
    }

    std::unique_ptr<VideoEncoderFactory> VideoFactoryConfig::CreateVideoEncoderFactory() {
        return absl::make_unique<VideoEncoderFactory>(encoders);
    }

    std::unique_ptr<VideoDecoderFactory> VideoFactoryConfig::CreateVideoDecoderFactory() {
        return absl::make_unique<VideoDecoderFactory>(decoders);
    }

    void VideoFactoryConfig::EnableH264Encoder(const bool enable) {
        allowH264Encoder = enable;
    }
} // wrtc
#endif