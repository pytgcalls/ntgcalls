//
// Created by Lauren on 20/11/26.
//

#include <cstring>
#include <wrtc/audio_factory/opus/opus_packet.hpp>
#include <wrtc/audio_factory/opus/opus_passthrough_decoder.hpp>

namespace wrtc::audio_factory {
    OpusPassthroughDecoder::OpusPassthroughDecoder(std::shared_ptr<utils::encoded::EncodedQueue> queue, const size_t channels): queue_(std::move(queue)), channels_(channels) {}

    void OpusPassthroughDecoder::Reset() {}

    int OpusPassthroughDecoder::PacketDuration(const uint8_t* encoded, const size_t encoded_len) const {
        return OpusPacket::samples(encoded, encoded_len);
    }

    int OpusPassthroughDecoder::SampleRateHz() const {
        return kSampleRate;
    }

    size_t OpusPassthroughDecoder::Channels() const {
        return channels_;
    }

    int OpusPassthroughDecoder::DecodeInternal(const uint8_t* encoded, const size_t encoded_len, int, int16_t* decoded, SpeechType* speech_type) {
        const int samples = PacketDuration(encoded, encoded_len);
        if (samples <= 0) {
            return -1;
        }
        queue_->push({{encoded, encoded + encoded_len}, {0, webrtc::kVideoRotation_0, 0, 0, models::Codec::Opus, true}});
        const auto total = static_cast<size_t>(samples) * channels_;
        std::memset(decoded, 0, total * sizeof(int16_t));
        *speech_type = kSpeech;
        return static_cast<int>(total);
    }
} // wrtc::audio_factory
