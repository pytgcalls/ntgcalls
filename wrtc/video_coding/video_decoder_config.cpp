//
// Created by Laky64 on 18/08/2023.
//

#include "video_decoder_config.hpp"

namespace wrtc {
    VideoDecoderConfig::VideoDecoderConfig(webrtc::VideoCodecType codec, DecoderCallback decoder) {
        this->codec = codec;
        this->decoder = std::move(decoder);
    }

    VideoDecoderConfig::VideoDecoderConfig(FormatsRetriever formatsRetriever, DecoderCallback decoder) {
        this->formatsRetriever = std::move(formatsRetriever);
        this->decoder = std::move(decoder);
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

    std::unique_ptr<webrtc::VideoDecoder> VideoDecoderConfig::CreateVideoCodec(const webrtc::SdpVideoFormat& format) {
       if (factory) {
           return factory->CreateVideoDecoder(format);
       } else {
           return decoder(format);
       }
    }

} // wrtc