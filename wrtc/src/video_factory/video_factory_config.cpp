//
// Created by Laky64 on 18/08/2023.
//

#include <wrtc/video_factory/video_factory_config.hpp>
#include <wrtc/video_factory/hardware/android/android.hpp>
#include <wrtc/video_factory/software/vlc/vlc.hpp>

namespace wrtc {

    VideoFactoryConfig::VideoFactoryConfig(void* jniEnv) {
        // Google (Software, VP9, VP8)
        google::addEncoders(encoders);
        google::addDecoders(decoders);

        // VLC (Software, AV1)
        vlc::addEncoders(encoders);
        vlc::addDecoders(decoders);

        // Android (Hardware, H264, VP8, VP9)
        android::addEncoders(encoders, jniEnv);
        android::addDecoders(decoders, jniEnv);

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