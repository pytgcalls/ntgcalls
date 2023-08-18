//
// Created by Laky64 on 18/08/2023.
//

#include "video_encoder_factory.hpp"

namespace wrtc {
    std::unique_ptr<webrtc::VideoEncoder>
    VideoEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat &format) {
        return internalVideo<webrtc::VideoEncoder>(encoders, format);
    }

    std::vector<webrtc::SdpVideoFormat> VideoEncoderFactory::GetSupportedFormats() const {
        return internalFormats(encoders);
    }
} // wrtc