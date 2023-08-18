//
// Created by Laky64 on 18/08/2023.
//

#include "video_decoder_factory.hpp"

namespace wrtc {
    std::unique_ptr<webrtc::VideoDecoder>
    VideoDecoderFactory::CreateVideoDecoder(const webrtc::SdpVideoFormat &format) {
        return internalVideo<webrtc::VideoDecoder>(decoders, format);
    }

    std::vector<webrtc::SdpVideoFormat> VideoDecoderFactory::GetSupportedFormats() const {
        return internalFormats(decoders);
    }
} // wrtc