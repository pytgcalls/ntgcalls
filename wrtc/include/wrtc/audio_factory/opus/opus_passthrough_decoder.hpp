//
// Created by Lauren on 20/11/26.
//

#pragma once
#include <optional>
#include <api/audio_codecs/audio_decoder.h>
#include <rtc_base/numerics/sequence_number_unwrapper.h>
#include <wrtc/utils/encoded/encoded_queue.hpp>

namespace wrtc::audio_factory {
    class OpusPassthroughDecoder final: public webrtc::AudioDecoder {
        static constexpr int kSampleRate = 48000;

        std::shared_ptr<utils::encoded::EncodedQueue> queue_;
        size_t channels_;
        webrtc::RtpTimestampUnwrapper unwrapper_;
        std::optional<int64_t> first_timestamp_;
        int64_t first_arrival_ms_ = 0;

        int64_t capture_time_ms(uint32_t timestamp);

    public:
        OpusPassthroughDecoder(std::shared_ptr<utils::encoded::EncodedQueue> queue, size_t channels);

        void Reset() override;

        std::vector<ParseResult> ParsePayload(webrtc::Buffer&& payload, uint32_t timestamp) override;

        int PacketDuration(const uint8_t* encoded, size_t encoded_len) const override;

        int SampleRateHz() const override;

        size_t Channels() const override;

    protected:
        int DecodeInternal(const uint8_t* encoded, size_t encoded_len, int sample_rate_hz, int16_t* decoded, SpeechType* speech_type) override;
    };
} // wrtc::audio_factory
