//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <api/video_codecs/video_decoder_factory.h>
#include <api/video_codecs/video_encoder_factory.h>
#include <wrtc/utils/encoded/encoded_queue_registry.hpp>

namespace wrtc::video_factory {
    class PassthroughEncoderFactory final: public webrtc::VideoEncoderFactory, public utils::encoded::EncodedQueueRegistry {
        std::unique_ptr<webrtc::VideoEncoderFactory> factory_;
        webrtc::VideoDecoderFactory* decoder_factory_;

    public:
        PassthroughEncoderFactory(std::unique_ptr<webrtc::VideoEncoderFactory> factory, webrtc::VideoDecoderFactory* decoder_factory);

        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

        CodecSupport QueryCodecSupport(const webrtc::SdpVideoFormat& format, std::optional<std::string> scalability_mode, std::optional<webrtc::Resolution> resolution) const override;

        std::unique_ptr<webrtc::VideoEncoder> Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) override;
    };
} // wrtc::video_factory
