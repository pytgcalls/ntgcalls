//
// Created by Laky64 on 18/08/2023.
//

#include "video_factory_config.hpp"

#include "software/vlc/vlc.hpp"

namespace wrtc {

    VideoFactoryConfig::VideoFactoryConfig() {
        // Google (Software, VP9, VP8)
        google::addEncoders(encoders);
        google::addDecoders(decoders);

        // VLC (Software, AV1)
        vlc::addEncoders(encoders);
        vlc::addDecoders(decoders);

        // NVCODEC (Hardware, VP8, VP9, H264)
        // TODO: @Laky-64 Add NVCODEC encoder-decoder when available
    }

    std::unique_ptr<VideoEncoderFactory> VideoFactoryConfig::CreateVideoEncoderFactory() {
        return absl::make_unique<VideoEncoderFactory>(encoders);
    }

    std::unique_ptr<VideoDecoderFactory> VideoFactoryConfig::CreateVideoDecoderFactory() {
        return absl::make_unique<VideoDecoderFactory>(decoders);
    }

} // wrtc