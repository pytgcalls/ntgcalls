//
// Created by Lauren on 18/08/23.
//

#ifndef IS_ANDROID
#include <wrtc/video_factory/video_factory_config.hpp>
#include <wrtc/video_factory/software/google/google.hpp>
#include <wrtc/video_factory/software/openh264/openh264.hpp>
#include <wrtc/video_factory/software/vlc/vlc.hpp>

namespace wrtc::video_factory {

    VideoFactoryConfig::VideoFactoryConfig() {
        // Google (Software, VP9, VP8)
        google::add_encoders(encoders);
        google::add_decoders(decoders);

        // VLC (Software, AV1)
        vlc::add_encoders(encoders);
        vlc::add_decoders(decoders);

        // OpenH264 (Software, H264)
        openh264::add_encoders(encoders);
        openh264::add_decoders(decoders);

        // NVCODEC (Hardware, VP8, VP9, H264)
        // TODO: @Lauren Add NVCODEC encoder-decoder when available
    }

    std::unique_ptr<VideoEncoderFactory> VideoFactoryConfig::CreateVideoEncoderFactory() {
        return absl::make_unique<VideoEncoderFactory>(encoders);
    }

    std::unique_ptr<VideoDecoderFactory> VideoFactoryConfig::CreateVideoDecoderFactory() {
        return absl::make_unique<VideoDecoderFactory>(decoders);
    }

} // wrtc::video_factory
#endif