//
// Created by Lauren on 20/11/26.
//

#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <wrtc/audio_factory/opus/opus_packet.hpp>
#include <wrtc/audio_factory/audio_decoder_factory.hpp>
#include <wrtc/audio_factory/opus/opus_passthrough_decoder.hpp>

namespace wrtc::audio_factory {
    AudioDecoderFactory::AudioDecoderFactory(): builtin_(webrtc::CreateBuiltinAudioDecoderFactory()) {}

    std::vector<webrtc::AudioCodecSpec> AudioDecoderFactory::GetSupportedDecoders() {
        return builtin_->GetSupportedDecoders();
    }

    bool AudioDecoderFactory::IsSupportedDecoder(const webrtc::SdpAudioFormat& format) {
        return builtin_->IsSupportedDecoder(format);
    }

    std::unique_ptr<webrtc::AudioDecoder> AudioDecoderFactory::Create(const webrtc::Environment& env, const webrtc::SdpAudioFormat& format) {
        if (auto queue = find_queue(format.parameters)) {
            return std::make_unique<OpusPassthroughDecoder>(std::move(queue), OpusPacket::channels(format));
        }
        return builtin_->Create(env, format);
    }
} // wrtc::audio_factory
