//
// Created by Lauren on 18/08/23.
//

#include <wrtc/video_factory/video_decoder_config.hpp>

namespace wrtc::video_factory {
    VideoDecoderConfig::VideoDecoderConfig(const webrtc::VideoCodecType codec, DecoderCallback create_video_decoder) {
        this->codec_ = codec;
        this->decoder_ = std::move(create_video_decoder);
    }

    VideoDecoderConfig::VideoDecoderConfig(FormatsRetriever get_supported_formats, DecoderCallback create_video_decoder) {
        this->formats_retriever_ = std::move(get_supported_formats);
        this->decoder_ = std::move(create_video_decoder);
    }

    bool VideoDecoderConfig::is_internal() const {
        return factory_ != nullptr;
    }

    std::vector<webrtc::SdpVideoFormat> VideoDecoderConfig::get_internal_formats() const {
        return factory_->GetSupportedFormats();
    }

    VideoDecoderConfig::~VideoDecoderConfig() {
        factory_ = nullptr;
        formats_retriever_ = nullptr;
        decoder_ = nullptr;
    }

    std::unique_ptr<webrtc::VideoDecoder> VideoDecoderConfig::create_video_codec(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) const {
       if (factory_) {
           return factory_->Create(env, format);
       }
       return decoder_(format);
    }

} // wrtc::video_factory