//
// Created by Lauren on 20/11/26.
//

#include <wrtc/video_factory/passthrough/passthrough_decoder_factory.hpp>
#include <wrtc/video_factory/passthrough/passthrough_video_decoder.hpp>

namespace wrtc::video_factory {
    PassthroughDecoderFactory::PassthroughDecoderFactory(std::unique_ptr<webrtc::VideoDecoderFactory> factory): factory_(std::move(factory)) {}

    std::vector<webrtc::SdpVideoFormat> PassthroughDecoderFactory::GetSupportedFormats() const {
        return factory_->GetSupportedFormats();
    }

    webrtc::VideoDecoderFactory::CodecSupport PassthroughDecoderFactory::QueryCodecSupport(const webrtc::SdpVideoFormat& format, const bool reference_scaling, const std::optional<webrtc::Resolution> resolution) const {
        return factory_->QueryCodecSupport(format, reference_scaling, resolution);
    }

    std::unique_ptr<webrtc::VideoDecoder> PassthroughDecoderFactory::Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) {
        if (auto queue = find_queue(format.parameters)) {
            return std::make_unique<PassthroughVideoDecoder>(std::move(queue), format.name);
        }
        return factory_->Create(env, format);
    }

    webrtc::VideoDecoderFactory* PassthroughDecoderFactory::inner() const {
        return factory_.get();
    }
} // wrtc::video_factory
