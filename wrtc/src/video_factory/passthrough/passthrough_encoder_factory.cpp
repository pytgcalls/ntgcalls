//
// Created by Lauren on 20/11/26.
//

#include <wrtc/video_factory/passthrough/passthrough_encoder_factory.hpp>
#include <wrtc/video_factory/passthrough/passthrough_video_encoder.hpp>

namespace wrtc::video_factory {
    PassthroughEncoderFactory::PassthroughEncoderFactory(std::unique_ptr<webrtc::VideoEncoderFactory> factory, webrtc::VideoDecoderFactory* decoder_factory): factory_(std::move(factory)), decoder_factory_(decoder_factory) {}

    std::vector<webrtc::SdpVideoFormat> PassthroughEncoderFactory::GetSupportedFormats() const {
        return factory_->GetSupportedFormats();
    }

    webrtc::VideoEncoderFactory::CodecSupport PassthroughEncoderFactory::QueryCodecSupport(const webrtc::SdpVideoFormat& format, const std::optional<std::string> scalability_mode, const std::optional<webrtc::Resolution> resolution) const {
        return factory_->QueryCodecSupport(format, scalability_mode, resolution);
    }

    std::unique_ptr<webrtc::VideoEncoder> PassthroughEncoderFactory::Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) {
        auto queue = find_queue(format.parameters);
        auto inner_format = format;
        inner_format.parameters.erase(kPassthroughParam);
        auto encoder = factory_->Create(env, inner_format);
        if (!queue || !encoder) {
            return encoder;
        }
        return std::make_unique<PassthroughVideoEncoder>(env, std::move(queue), std::move(encoder), decoder_factory_, inner_format);
    }
} // wrtc::video_factory
