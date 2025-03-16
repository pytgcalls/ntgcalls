//
// Created by Laky64 on 18/08/2023.
//

#include <wrtc/video_factory/video_encoder_config.hpp>

namespace wrtc {

    VideoEncoderConfig::VideoEncoderConfig(const webrtc::VideoCodecType codec, EncoderCallback encoder, const int alignment) {
        this->codec = codec;
        this->encoder = std::move(encoder);
        this->alignment = alignment;
    }

    VideoEncoderConfig::VideoEncoderConfig(FormatsRetriever formatsRetriever, EncoderCallback encoder, const int alignment) {
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

    std::unique_ptr<webrtc::VideoEncoder> VideoEncoderConfig::CreateVideoCodec(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) const {
        if (factory) {
            return factory->Create(env, format);
        }
        return encoder(format);
    }

    VideoEncoderConfig::~VideoEncoderConfig() {
        factory = nullptr;
        formatsRetriever = nullptr;
        encoder = nullptr;
    }
} // wrtc