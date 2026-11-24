//
// Created by Lauren on 20/11/26.
//

#include <cstring>
#include <rtc_base/time_utils.h>
#include <wrtc/audio_factory/opus/opus_packet.hpp>
#include <wrtc/audio_factory/opus/opus_passthrough_decoder.hpp>

namespace wrtc::audio_factory {
    OpusPassthroughDecoder::OpusPassthroughDecoder(std::shared_ptr<utils::encoded::EncodedQueue> queue, const size_t channels): queue_(std::move(queue)), channels_(channels) {}

    void OpusPassthroughDecoder::Reset() {}

    std::vector<webrtc::AudioDecoder::ParseResult> OpusPassthroughDecoder::ParsePayload(webrtc::Buffer&& payload, const uint32_t timestamp) {
        if (PacketDuration(payload.data(), payload.size()) > 0) {
            queue_->push({{payload.data(), payload.data() + payload.size()}, {capture_time_ms(timestamp), webrtc::kVideoRotation_0, 0, 0, models::Codec::Opus, true}, webrtc::TimeMillis()});
        }
        return AudioDecoder::ParsePayload(std::move(payload), timestamp);
    }

    int64_t OpusPassthroughDecoder::capture_time_ms(const uint32_t timestamp) {
        const int64_t unwrapped = unwrapper_.Unwrap(timestamp);
        if (!first_timestamp_) {
            first_timestamp_ = unwrapped;
            first_arrival_ms_ = webrtc::TimeMillis();
        }
        return first_arrival_ms_ + (unwrapped - *first_timestamp_) * 1000 / kSampleRate;
    }

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
        const auto total = static_cast<size_t>(samples) * channels_;
        std::memset(decoded, 0, total * sizeof(int16_t));
        *speech_type = kSpeech;
        return static_cast<int>(total);
    }
} // wrtc::audio_factory
