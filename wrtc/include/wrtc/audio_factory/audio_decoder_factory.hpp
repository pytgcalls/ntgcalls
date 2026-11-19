//
// Created by Lauren on 20/11/26.
//

#pragma once
#include <api/audio_codecs/audio_decoder_factory.h>
#include <wrtc/utils/encoded/encoded_queue_registry.hpp>

namespace wrtc::audio_factory {
    class AudioDecoderFactory: public webrtc::AudioDecoderFactory, public utils::encoded::EncodedQueueRegistry {
        webrtc::scoped_refptr<webrtc::AudioDecoderFactory> builtin_;

    public:
        AudioDecoderFactory();

        std::vector<webrtc::AudioCodecSpec> GetSupportedDecoders() override;

        bool IsSupportedDecoder(const webrtc::SdpAudioFormat& format) override;

        std::unique_ptr<webrtc::AudioDecoder> Create(const webrtc::Environment& env, const webrtc::SdpAudioFormat& format) override;
    };
} // wrtc::audio_factory
