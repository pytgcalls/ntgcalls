//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <memory>
#include <ntgcalls/media/codecs/opus_packet_encoder.hpp>
#include <ntgcalls/media/codecs/pcm_resampler.hpp>
#include <ntgcalls/media/mixers/mixer.hpp>

struct OpusDecoder;

namespace ntgcalls::media::mixers {

    class AudioMixer final: public Mixer {
        static constexpr int kPacketSamples = codecs::OpusPacketEncoder::kFrameSamples;
        static constexpr int kSamplesPerMs = 48;

        struct Source {
            OpusDecoder* decoder = nullptr;
            std::optional<int64_t> position;
        };

        bool opus_;
        bool encoded_input_;
        std::mutex mutex_;
        std::optional<int64_t> origin_ms_;
        int64_t newest_ = 0;
        std::optional<int64_t> source_ssrc_;
        bool transcoding_ = false;
        std::map<int64_t, Source> sources_;
        std::unique_ptr<codecs::OpusPacketEncoder> encoder_;
        std::unique_ptr<codecs::PcmResampler> resampler_;
        std::deque<int32_t> mix_;
        int64_t mix_position_ = 0;

        uint8_t channels() const;

        uint32_t sample_rate() const;

        int64_t mix_delay() const;

        void ensure_encoder();

        bool ensure_decoder(Source& source) const;

        void write_packet(const bytes::binary& packet, int samples) const;

        void start_transcoding(int64_t ssrc);

        void forward(Source& source, const bytes::binary& packet, int64_t position, int samples);

        void transcode(Source& source, const bytes::binary& packet, int64_t position, int samples);

        void mix(int64_t position, const std::vector<int16_t>& pcm);

        void encode_mix(bool all);

        void write_mixed_pcm(const std::vector<int16_t>& pcm, bool all);

        void send_pcm(const std::vector<wrtc::models::Frame>& frames) const;

        void write_pcm(bytes::unique_binary data, size_t size, uint32_t rate, std::chrono::microseconds duration) const;

    public:
        static constexpr auto kKind = containers::MediaKind::Audio;

        AudioMixer(BaseSink* sink, CodecFilter supports, PacketCallback packet_callback);

        ~AudioMixer() override;

        void send_frames(std::vector<wrtc::models::Frame> frames) override;

        void tick() override;

        [[nodiscard]] std::chrono::nanoseconds tick_interval() const override;

        void flush() override;

        static bytes::unique_binary mix_pcm(const std::vector<wrtc::models::Frame>& frames, int64_t frame_size);
    };

} // ntgcalls::media::mixers
