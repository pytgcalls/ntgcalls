//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <api/video_codecs/video_decoder_factory.h>
#include <wrtc/utils/encoded/encoded_queue_registry.hpp>

namespace wrtc::video_factory {
    class PassthroughDecoderFactory final: public webrtc::VideoDecoderFactory, public utils::encoded::EncodedQueueRegistry {
        std::unique_ptr<webrtc::VideoDecoderFactory> factory_;

    public:
        explicit PassthroughDecoderFactory(std::unique_ptr<webrtc::VideoDecoderFactory> factory);

        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

        CodecSupport QueryCodecSupport(const webrtc::SdpVideoFormat& format, bool reference_scaling, std::optional<webrtc::Resolution> resolution) const override;

        std::unique_ptr<webrtc::VideoDecoder> Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) override;

        [[nodiscard]] webrtc::VideoDecoderFactory* inner() const;
    };
} // wrtc::video_factory
