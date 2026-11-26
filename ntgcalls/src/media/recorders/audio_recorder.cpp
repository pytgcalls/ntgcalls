//
// Created by Lauren on 20/11/26.
//

#include <algorithm>
#include <array>
#include <ranges>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/audio_sink.hpp>
#include <ntgcalls/media/recorders/audio_recorder.hpp>
#include <rtc_base/logging.h>
#include <third_party/opus/src/include/opus.h>
#include <wrtc/audio_factory/opus/opus_jitter_buffer.hpp>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

namespace ntgcalls::media::recorders {
    AudioRecorder::AudioRecorder(BaseSink* sink, CodecFilter supports, PacketCallback packet_callback): Recorder(sink, std::move(supports), std::move(packet_callback)) {}

    AudioRecorder::~AudioRecorder() {
        for (const auto& source : sources_ | std::views::values) {
            if (source.decoder) {
                opus_decoder_destroy(source.decoder);
            }
        }
        if (encoder_) {
            opus_encoder_destroy(encoder_);
        }
    }

    uint8_t AudioRecorder::channels() const {
        const auto audio_sink = dynamic_cast<AudioSink*>(sink_);
        const auto description = audio_sink ? audio_sink->get_config() : std::nullopt;
        return description ? description->channel_count : 2;
    }

    int64_t AudioRecorder::mix_delay() const {
        const auto audio_sink = dynamic_cast<AudioSink*>(sink_);
        const auto description = audio_sink ? audio_sink->get_config() : std::nullopt;
        const auto max_delay = description && description->max_delay_ms ? static_cast<int64_t>(*description->max_delay_ms) : wrtc::audio_factory::OpusJitterBuffer::kDefaultMaxDelayMs;
        return max_delay * kSamplesPerMs;
    }

    void AudioRecorder::send_frames(std::vector<wrtc::models::Frame> frames) {
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
            } else if (!transcoding_ && *source_ssrc_ != frame.ssrc) {
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

    void AudioRecorder::start_transcoding() {
        RTC_LOG(LS_INFO) << "More than one participant, re-encoding the Opus recording";
        transcoding_ = true;
        ensure_encoder();
        mix_position_ = sources_[*source_ssrc_].position.value_or(0);
        mix_.clear();
    }

    void AudioRecorder::forward(Source& source, const bytes::binary& packet, const int64_t position, const int samples) {
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
                write_packet(silence_packet_, kPacketSamples);
            }
            *source.position += packets * kPacketSamples;
        }
        write_packet(packet, samples);
        *source.position += samples;
    }

    void AudioRecorder::transcode(Source& source, const bytes::binary& packet, const int64_t position, const int samples) {
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

    void AudioRecorder::mix(const int64_t position, const std::vector<int16_t>& pcm) {
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

    void AudioRecorder::encode_mix(const bool all) {
        const size_t block = static_cast<size_t>(kPacketSamples) * channels();
        while (!mix_.empty() && (all || mix_position_ + kPacketSamples <= newest_ - mix_delay())) {
            std::vector<int16_t> pcm(block, 0);
            const size_t available = std::min(block, mix_.size());
            for (size_t i = 0; i < available; i++) {
                pcm[i] = static_cast<int16_t>(std::clamp(mix_[i], -32768, 32767));
            }
            mix_.erase(mix_.begin(), mix_.begin() + static_cast<std::ptrdiff_t>(available));
            std::array<uint8_t, 4000> packet{};
            const int size = opus_encode(encoder_, pcm.data(), kPacketSamples, packet.data(), static_cast<opus_int32>(packet.size()));
            write_packet(size > 0 ? bytes::binary{packet.data(), packet.data() + size} : silence_packet_, kPacketSamples);
            mix_position_ += kPacketSamples;
        }
    }

    void AudioRecorder::ensure_encoder() {
        if (encoder_) {
            return;
        }
        const auto count = channels();
        int error = 0;
        encoder_ = opus_encoder_create(wrtc::audio_factory::OpusPacket::kSampleRate, count, OPUS_APPLICATION_AUDIO, &error);
        if (error != OPUS_OK) {
            encoder_ = nullptr;
            throw FileError("Unable to create the Opus encoder");
        }
        const auto audio_sink = dynamic_cast<AudioSink*>(sink_);
        if (const auto description = audio_sink ? audio_sink->get_config() : std::nullopt; description && description->bitrate) {
            opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(static_cast<opus_int32>(*description->bitrate)));
        }
        const std::vector<int16_t> silence(static_cast<size_t>(kPacketSamples) * count, 0);
        std::array<uint8_t, 4000> packet{};
        const int size = opus_encode(encoder_, silence.data(), kPacketSamples, packet.data(), static_cast<opus_int32>(packet.size()));
        silence_packet_.assign(packet.data(), packet.data() + std::max(size, 0));
    }

    bool AudioRecorder::ensure_decoder(Source& source) const {
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

    void AudioRecorder::write_packet(const bytes::binary& packet, const int samples) const {
        packet_callback_(
            {wrtc::models::Codec::Opus, channels(), 0, 0},
            {bytes::const_span(packet.data(), packet.size()), 0, std::chrono::microseconds(static_cast<int64_t>(samples) * 1000 / kSamplesPerMs), true}
        );
    }

    std::chrono::nanoseconds AudioRecorder::tick_interval() const {
        return sink_->frame_time() + std::chrono::milliseconds(20);
    }

    void AudioRecorder::flush() {
        const std::lock_guard lock(mutex_);
        if (transcoding_) {
            encode_mix(true);
        }
    }
} // ntgcalls::media::recorders
