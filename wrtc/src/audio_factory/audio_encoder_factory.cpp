//
// Created by Lauren on 19/11/26.
//

#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <wrtc/audio_factory/opus/opus_packet.hpp>
#include <wrtc/audio_factory/audio_encoder_factory.hpp>
#include <wrtc/audio_factory/opus/opus_passthrough_encoder.hpp>

namespace wrtc::audio_factory {
    AudioEncoderFactory::AudioEncoderFactory(): builtin_(webrtc::CreateBuiltinAudioEncoderFactory()) {}

    std::vector<webrtc::AudioCodecSpec> AudioEncoderFactory::GetSupportedEncoders() {
        return builtin_->GetSupportedEncoders();
    }

    std::optional<webrtc::AudioCodecInfo> AudioEncoderFactory::QueryAudioEncoder(const webrtc::SdpAudioFormat& format) {
        return builtin_->QueryAudioEncoder(format);
    }

    std::unique_ptr<webrtc::AudioEncoder> AudioEncoderFactory::Create(const webrtc::Environment& env, const webrtc::SdpAudioFormat& format, const Options options) {
        auto queue = find_queue(format.parameters);
        if (!format.parameters.contains(kPassthroughParam)) {
            return builtin_->Create(env, format, options);
        }
        auto opus_format = format;
        opus_format.parameters.erase(kPassthroughParam);
        auto encoder = builtin_->Create(env, opus_format, options);
        if (!queue || !encoder) {
            return encoder;
        }
        std::optional<uint32_t> max_bitrate;
        if (const auto bitrate = format.parameters.find("maxaveragebitrate"); bitrate != format.parameters.end()) {
            max_bitrate = static_cast<uint32_t>(std::stoul(bitrate->second));
        }
        return std::make_unique<OpusPassthroughEncoder>(std::move(queue), std::move(encoder), options.payload_type, OpusPacket::channels(format), max_bitrate);
    }
} // wrtc::audio_factory
