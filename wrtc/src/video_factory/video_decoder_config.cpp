//
// Created by Laky64 on 18/08/2023.
//

#include <wrtc/video_factory/video_decoder_config.hpp>

namespace wrtc {
    VideoDecoderConfig::VideoDecoderConfig(const webrtc::VideoCodecType codec, DecoderCallback createVideoDecoder) {
        this->codec = codec;
        this->decoder = std::move(createVideoDecoder);
    }

    VideoDecoderConfig::VideoDecoderConfig(FormatsRetriever getSupportedFormats, DecoderCallback createVideoDecoder) {
        this->formatsRetriever = std::move(getSupportedFormats);
        this->decoder = std::move(createVideoDecoder);
    }

    bool VideoDecoderConfig::isInternal() {
        return factory != nullptr;
    }

    std::vector<webrtc::SdpVideoFormat> VideoDecoderConfig::getInternalFormats() {
        return factory->GetSupportedFormats();
    }

    VideoDecoderConfig::~VideoDecoderConfig() {
        factory = nullptr;
        formatsRetriever = nullptr;
        decoder = nullptr;
    }

    std::unique_ptr<webrtc::VideoDecoder> VideoDecoderConfig::CreateVideoCodec(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) const {
       if (factory) {
           return factory->Create(env, format);
       }
       return decoder(format);
    }

} // wrtc