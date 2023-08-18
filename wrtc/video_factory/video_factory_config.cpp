//
// Created by Laky64 on 18/08/2023.
//

#include "video_factory_config.hpp"

namespace wrtc {

    std::unique_ptr<VideoEncoderFactory> VideoFactoryConfig::CreateVideoEncoderFactory() {
        return absl::make_unique<VideoEncoderFactory>(encoders);
    }

    std::unique_ptr<VideoDecoderFactory> VideoFactoryConfig::CreateVideoDecoderFactory() {
        return absl::make_unique<VideoDecoderFactory>(decoders);
    }

} // wrtc