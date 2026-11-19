//
// Created by Lauren on 20/11/26.
//

#pragma once
#include <api/audio_codecs/audio_decoder.h>
#include <wrtc/utils/encoded/encoded_queue.hpp>

namespace wrtc::audio_factory {
    class OpusPassthroughDecoder final: public webrtc::AudioDecoder {
        static constexpr int kSampleRate = 48000;

        std::shared_ptr<utils::encoded::EncodedQueue> queue_;
        size_t channels_;

    public:
        OpusPassthroughDecoder(std::shared_ptr<utils::encoded::EncodedQueue> queue, size_t channels);

        void Reset() override;

        int PacketDuration(const uint8_t* encoded, size_t encoded_len) const override;

        int SampleRateHz() const override;

        size_t Channels() const override;

    protected:
        int DecodeInternal(const uint8_t* encoded, size_t encoded_len, int sample_rate_hz, int16_t* decoded, SpeechType* speech_type) override;
    };
} // wrtc::audio_factory
