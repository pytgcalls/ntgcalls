//
// Created by Lauren on 27/11/26.
//

#include <array>
#include <vector>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/codecs/opus_packet_encoder.hpp>
#include <third_party/opus/src/include/opus.h>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

namespace ntgcalls::media::codecs {
    OpusPacketEncoder::OpusPacketEncoder(const uint8_t channels, const std::optional<uint32_t> bitrate): channels_(channels) {
        int error = 0;
        encoder_ = opus_encoder_create(wrtc::audio_factory::OpusPacket::kSampleRate, channels, OPUS_APPLICATION_AUDIO, &error);
        if (error != OPUS_OK) {
            encoder_ = nullptr;
            throw FileError("Unable to create the Opus encoder");
        }
        if (bitrate) {
            opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(static_cast<opus_int32>(*bitrate)));
        }
        const std::vector<int16_t> silence(static_cast<size_t>(kFrameSamples) * channels_, 0);
        silence_ = encode(silence.data());
    }

    OpusPacketEncoder::~OpusPacketEncoder() {
        if (encoder_) {
            opus_encoder_destroy(encoder_);
        }
    }

    bytes::binary OpusPacketEncoder::encode(const int16_t* pcm) const {
        std::array<uint8_t, 4000> packet{};
        const int size = opus_encode(encoder_, pcm, kFrameSamples, packet.data(), static_cast<opus_int32>(packet.size()));
        if (size <= 0) {
            return silence_;
        }
        return {packet.data(), packet.data() + size};
    }

    const bytes::binary& OpusPacketEncoder::silence() const {
        return silence_;
    }
} // ntgcalls::media::codecs
