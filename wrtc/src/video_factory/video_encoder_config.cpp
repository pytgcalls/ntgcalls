//
// Created by Lauren on 18/08/23.
//

#include <wrtc/video_factory/video_encoder_config.hpp>

namespace wrtc::video_factory {

    VideoEncoderConfig::VideoEncoderConfig(const webrtc::VideoCodecType codec, EncoderCallback encoder, const int alignment) {
        this->codec_ = codec;
        this->encoder_ = std::move(encoder);
        this->alignment_ = alignment;
    }

    VideoEncoderConfig::VideoEncoderConfig(FormatsRetriever formats_retriever, EncoderCallback encoder, const int alignment) {
        this->formats_retriever_ = std::move(formats_retriever);
        this->encoder_ = std::move(encoder);
        this->alignment_ = alignment;
    }

    bool VideoEncoderConfig::is_internal() const {
        return factory_ != nullptr;
    }

    std::vector<webrtc::SdpVideoFormat> VideoEncoderConfig::get_internal_formats() const {
        return factory_->GetSupportedFormats();
    }

    std::unique_ptr<webrtc::VideoEncoder> VideoEncoderConfig::create_video_codec(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) const {
        if (factory_) {
            return factory_->Create(env, format);
        }
        return encoder_(format);
    }

    VideoEncoderConfig::~VideoEncoderConfig() {
        factory_ = nullptr;
        formats_retriever_ = nullptr;
        encoder_ = nullptr;
    }
} // wrtc::video_factory
