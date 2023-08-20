//
// Created by Laky64 on 18/08/2023.
//

#include "video_encoder_config.hpp"

namespace wrtc {

    VideoEncoderConfig::VideoEncoderConfig(webrtc::VideoCodecType codec, EncoderCallback encoder, int alignment) {
        this->codec = codec;
        this->encoder = std::move(encoder);
        this->alignment = alignment;
    }

    VideoEncoderConfig::VideoEncoderConfig(FormatsRetriever formatsRetriever, EncoderCallback encoder, int alignment) {
        this->formatsRetriever = std::move(formatsRetriever);
        this->encoder = std::move(encoder);
        this->alignment = alignment;
    }

    bool VideoEncoderConfig::isInternal() {
        return factory != nullptr;
    }

    std::vector<webrtc::SdpVideoFormat> VideoEncoderConfig::getInternalFormats() {
        return factory->GetSupportedFormats();
    }

    std::unique_ptr<webrtc::VideoEncoder> VideoEncoderConfig::CreateVideoCodec(const webrtc::SdpVideoFormat& format) {
        if (factory) {
            return factory->CreateVideoEncoder(format);
        } else {
            return encoder(format);
        }
    }

    VideoEncoderConfig::~VideoEncoderConfig() {
        factory = nullptr;
        formatsRetriever = nullptr;
        encoder = nullptr;
    }
} // wrtc