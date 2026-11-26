//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <ntgcalls/media/mixers/mixer.hpp>

struct OpusDecoder;
struct OpusEncoder;

namespace ntgcalls::media::mixers {

    class AudioMixer final: public Mixer {
        static constexpr int kPacketSamples = 960;
        static constexpr int kSamplesPerMs = 48;

        struct Source {
            OpusDecoder* decoder = nullptr;
            std::optional<int64_t> position;
        };

        bool opus_;
        std::mutex mutex_;
        std::optional<int64_t> origin_ms_;
        int64_t newest_ = 0;
        std::optional<int64_t> source_ssrc_;
        bool transcoding_ = false;
        std::map<int64_t, Source> sources_;
        OpusEncoder* encoder_ = nullptr;
        std::deque<int32_t> mix_;
        int64_t mix_position_ = 0;
        bytes::binary silence_packet_;

        uint8_t channels() const;

        int64_t mix_delay() const;

        void ensure_encoder();

        bool ensure_decoder(Source& source) const;

        void write_packet(const bytes::binary& packet, int samples) const;

        void start_transcoding();

        void forward(Source& source, const bytes::binary& packet, int64_t position, int samples);

        void transcode(Source& source, const bytes::binary& packet, int64_t position, int samples);

        void mix(int64_t position, const std::vector<int16_t>& pcm);

        void encode_mix(bool all);

        void send_pcm(const std::vector<wrtc::models::Frame>& frames) const;

        void write_pcm(bytes::unique_binary data, size_t size) const;

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
