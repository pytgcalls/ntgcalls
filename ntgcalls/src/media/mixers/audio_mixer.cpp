//
// Created by Lauren on 20/11/26.
//

#include <algorithm>
#include <ranges>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/audio_sink.hpp>
#include <ntgcalls/media/mixers/audio_mixer.hpp>
#include <rtc_base/logging.h>
#include <third_party/opus/src/include/opus.h>
#include <wrtc/audio_factory/opus/opus_jitter_buffer.hpp>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

namespace ntgcalls::media::mixers {
    AudioMixer::AudioMixer(BaseSink* sink, CodecFilter supports, PacketCallback packet_callback): Mixer(sink, std::move(supports), std::move(packet_callback)), opus_(supports_(wrtc::models::Codec::Opus)) {
        const auto audio_sink = dynamic_cast<AudioSink*>(sink_);
        const auto description = audio_sink ? audio_sink->get_config() : std::nullopt;
        encoded_input_ = description && description->is_encoded();
    }

    AudioMixer::~AudioMixer() {
        for (const auto& source : sources_ | std::views::values) {
            if (source.decoder) {
                opus_decoder_destroy(source.decoder);
            }
        }
    }

    uint8_t AudioMixer::channels() const {
        const auto audio_sink = dynamic_cast<AudioSink*>(sink_);
        const auto description = audio_sink ? audio_sink->get_config() : std::nullopt;
        return description ? description->channel_count : 2;
    }

    uint32_t AudioMixer::sample_rate() const {
        const auto audio_sink = dynamic_cast<AudioSink*>(sink_);
        const auto description = audio_sink ? audio_sink->get_config() : std::nullopt;
        return description ? description->sample_rate : wrtc::audio_factory::OpusPacket::kSampleRate;
    }

    int64_t AudioMixer::mix_delay() const {
        const auto audio_sink = dynamic_cast<AudioSink*>(sink_);
        const auto description = audio_sink ? audio_sink->get_config() : std::nullopt;
        const auto max_delay = description && description->max_delay_ms ? static_cast<int64_t>(*description->max_delay_ms) : wrtc::audio_factory::OpusJitterBuffer::kDefaultMaxDelayMs;
        return max_delay * kSamplesPerMs;
    }

    void AudioMixer::send_frames(const std::vector<wrtc::models::Frame> frames) {
        if (!opus_ && !encoded_input_) {
            send_pcm(frames);
            return;
        }
        const std::lock_guard lock(mutex_);
        for (const auto& frame : frames) {
            const int samples = wrtc::audio_factory::OpusPacket::samples(frame.data.data(), frame.data.size());
            if (samples <= 0) {
                continue;
            }
            const int64_t timestamp = frame.frame_data.absolute_capture_timestamp_ms;
            if (!origin_ms_) {
                origin_ms_ = timestamp;
            }
            const int64_t position = (timestamp - *origin_ms_) * kSamplesPerMs;
            if (!source_ssrc_) {
                source_ssrc_ = frame.ssrc;
            }
            if (!transcoding_ && (!opus_ || *source_ssrc_ != frame.ssrc)) {
                start_transcoding();
            }
            newest_ = std::max(newest_, position + samples);
            if (transcoding_) {
                transcode(sources_[frame.ssrc], frame.data, position, samples);
            } else {
                forward(sources_[frame.ssrc], frame.data, position, samples);
            }
        }
        if (transcoding_) {
            encode_mix(false);
        }
    }

    void AudioMixer::start_transcoding() {
        RTC_LOG(LS_INFO) << "More than one participant, re-encoding the Opus recording";
        transcoding_ = true;
        if (opus_) {
            ensure_encoder();
        }
        mix_position_ = sources_[*source_ssrc_].position.value_or(0);
        mix_.clear();
    }

    void AudioMixer::forward(Source& source, const bytes::binary& packet, const int64_t position, const int samples) {
        if (!source.position) {
            source.position = position;
        }
        const int64_t gap = position - *source.position;
        if (gap < -kPacketSamples / 2) {
            return;
        }
        if (gap > kPacketSamples / 2) {
            ensure_encoder();
            const int64_t packets = (gap + kPacketSamples / 2) / kPacketSamples;
            for (int64_t i = 0; i < packets; i++) {
                write_packet(encoder_->silence(), kPacketSamples);
            }
            *source.position += packets * kPacketSamples;
        }
        write_packet(packet, samples);
        *source.position += samples;
    }

    void AudioMixer::transcode(Source& source, const bytes::binary& packet, const int64_t position, const int samples) {
        if (!ensure_decoder(source)) {
            return;
        }
        if (!source.position) {
            source.position = position;
        }
        const int64_t gap = position - *source.position;
        if (gap < -kPacketSamples / 2) {
            return;
        }
        if (gap > kPacketSamples / 2) {
            opus_decoder_ctl(source.decoder, OPUS_RESET_STATE);
            source.position = position;
        }
        std::vector<int16_t> pcm(static_cast<size_t>(samples) * channels(), 0);
        if (opus_decode(source.decoder, packet.data(), static_cast<opus_int32>(packet.size()), pcm.data(), samples, 0) < 0) {
            std::ranges::fill(pcm, 0);
        }
        mix(*source.position, pcm);
        *source.position += samples;
    }

    void AudioMixer::mix(const int64_t position, const std::vector<int16_t>& pcm) {
        const auto count = static_cast<int64_t>(channels());
        const int64_t frames = static_cast<int64_t>(pcm.size()) / count;
        const int64_t offset = position - mix_position_;
        if (offset + frames <= 0) {
            return;
        }
        if (const auto needed = static_cast<size_t>((offset + frames) * count); mix_.size() < needed) {
            mix_.resize(needed, 0);
        }
        for (int64_t i = std::max<int64_t>(-offset, 0); i < frames; i++) {
            for (int64_t c = 0; c < count; c++) {
                mix_[static_cast<size_t>((offset + i) * count + c)] += pcm[static_cast<size_t>(i * count + c)];
            }
        }
    }

    void AudioMixer::encode_mix(const bool all) {
        const size_t block = static_cast<size_t>(kPacketSamples) * channels();
        while (!mix_.empty() && (all || mix_position_ + kPacketSamples <= newest_ - mix_delay())) {
            std::vector<int16_t> pcm(block, 0);
            const size_t available = std::min(block, mix_.size());
            for (size_t i = 0; i < available; i++) {
                pcm[i] = static_cast<int16_t>(std::clamp(mix_[i], -32768, 32767));
            }
            mix_.erase(mix_.begin(), mix_.begin() + static_cast<std::ptrdiff_t>(available));
            if (opus_) {
                write_packet(encoder_->encode(pcm.data()), kPacketSamples);
            } else {
                write_mixed_pcm(pcm, false);
            }
            mix_position_ += kPacketSamples;
        }
    }

    void AudioMixer::write_mixed_pcm(const std::vector<int16_t>& pcm, const bool all) {
        const auto rate = sample_rate();
        std::vector<int16_t> output;
        if (rate == static_cast<uint32_t>(wrtc::audio_factory::OpusPacket::kSampleRate)) {
            output = pcm;
        } else {
            if (!resampler_) {
                AVChannelLayout layout;
                av_channel_layout_default(&layout, channels());
                resampler_ = std::make_unique<codecs::PcmResampler>(layout, AV_SAMPLE_FMT_S16, wrtc::audio_factory::OpusPacket::kSampleRate, channels(), static_cast<int>(rate));
                av_channel_layout_uninit(&layout);
            }
            const auto* input = reinterpret_cast<const uint8_t*>(pcm.data());
            resampler_->convert(&input, static_cast<int>(pcm.size() / channels()), output);
            if (all) {
                resampler_->flush(output);
            }
        }
        if (output.empty()) {
            return;
        }
        const auto size = output.size() * sizeof(int16_t);
        const auto samples = static_cast<int64_t>(output.size() / channels());
        write_pcm(bytes::make_unique_binary(output.data(), size), size, rate, std::chrono::microseconds(samples * 1000000 / rate));
    }

    void AudioMixer::ensure_encoder() {
        if (encoder_) {
            return;
        }
        const auto audio_sink = dynamic_cast<AudioSink*>(sink_);
        const auto description = audio_sink ? audio_sink->get_config() : std::nullopt;
        encoder_ = std::make_unique<codecs::OpusPacketEncoder>(channels(), description ? description->bitrate : std::nullopt);
    }

    bool AudioMixer::ensure_decoder(Source& source) const {
        if (source.decoder) {
            return true;
        }
        int error = 0;
        source.decoder = opus_decoder_create(wrtc::audio_factory::OpusPacket::kSampleRate, channels(), &error);
        if (error != OPUS_OK) {
            source.decoder = nullptr;
            return false;
        }
        return true;
    }

    void AudioMixer::write_packet(const bytes::binary& packet, const int samples) const {
        packet_callback_(
            {wrtc::models::Codec::Opus, channels(), 0, 0, wrtc::audio_factory::OpusPacket::kSampleRate},
            {bytes::make_unique_binary(packet.data(), packet.size()), packet.size(), 0, std::chrono::microseconds(static_cast<int64_t>(samples) * 1000 / kSamplesPerMs), true}
        );
    }

    void AudioMixer::send_pcm(const std::vector<wrtc::models::Frame>& frames) const {
        const auto frame_size = sink_->frame_size();
        write_pcm(mix_pcm(frames, frame_size), static_cast<size_t>(frame_size), sample_rate(), std::chrono::duration_cast<std::chrono::microseconds>(sink_->frame_time()));
    }

    void AudioMixer::tick() {
        if (opus_ || encoded_input_) {
            return;
        }
        const auto size = static_cast<size_t>(sink_->frame_size());
        auto silence = bytes::make_unique_binary(size);
        std::memset(silence.get(), 0, size);
        write_pcm(std::move(silence), size, sample_rate(), std::chrono::duration_cast<std::chrono::microseconds>(sink_->frame_time()));
    }

    void AudioMixer::write_pcm(bytes::unique_binary data, const size_t size, const uint32_t rate, const std::chrono::microseconds duration) const {
        packet_callback_(
            {wrtc::models::Codec::PCM, channels(), 0, 0, rate},
            {std::move(data), size, 0, duration, true}
        );
    }

    std::chrono::nanoseconds AudioMixer::tick_interval() const {
        return sink_->frame_time() + std::chrono::milliseconds(20);
    }

    void AudioMixer::flush() {
        const std::lock_guard lock(mutex_);
        if (transcoding_) {
            encode_mix(true);
        }
        if (resampler_) {
            write_mixed_pcm({}, true);
        }
    }

    bytes::unique_binary AudioMixer::mix_pcm(const std::vector<wrtc::models::Frame>& frames, const int64_t frame_size) {
        bytes::unique_binary mixed_output = bytes::make_unique_binary(frame_size);
        std::fill_n(reinterpret_cast<int16_t*>(mixed_output.get()), frame_size / sizeof(int16_t), 0);

        const auto num_sources = std::max<size_t>(frames.size(), 1);
        for (size_t i = 0; i < frame_size / sizeof(int16_t); i++) {
            int32_t mixed_sample = 0;
            for (const auto& frame : frames) {
                if (frame.data.size() < static_cast<size_t>(frame_size)) {
                    continue;
                }
                mixed_sample += reinterpret_cast<const int16_t*>(frame.data.data())[i];
            }

            // Audio normalization
            mixed_sample /= static_cast<int32_t>(num_sources);

            // Clipping to a 16-bit signed integer range
            const auto mixed_output_samples = reinterpret_cast<int16_t*>(mixed_output.get());
            mixed_output_samples[i] = static_cast<int16_t>(std::clamp(mixed_sample, -32768, 32767));
        }
        return mixed_output;
    }
} // ntgcalls::media::mixers
