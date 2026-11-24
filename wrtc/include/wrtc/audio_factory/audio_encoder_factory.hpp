//
// Created by Lauren on 19/11/26.
//

#pragma once
#include <api/audio_codecs/audio_encoder_factory.h>
#include <wrtc/utils/encoded/encoded_queue_registry.hpp>

namespace wrtc::audio_factory {
    class AudioEncoderFactory: public webrtc::AudioEncoderFactory, public utils::encoded::EncodedQueueRegistry {
        webrtc::scoped_refptr<webrtc::AudioEncoderFactory> builtin_;

    public:
        AudioEncoderFactory();

        std::vector<webrtc::AudioCodecSpec> GetSupportedEncoders() override;

        std::optional<webrtc::AudioCodecInfo> QueryAudioEncoder(const webrtc::SdpAudioFormat& format) override;

        std::unique_ptr<webrtc::AudioEncoder> Create(const webrtc::Environment& env, const webrtc::SdpAudioFormat& format, Options options) override;
    };
} // wrtc::audio_factory
