//
// Created by Lauren on 07/10/24.
//

#include <ranges>
#include <algorithm>
#include <ntgcalls/io/audio_mixer.hpp>

namespace ntgcalls::io {
    AudioMixer::AudioMixer(media::BaseSink* sink): AudioWriter(sink) {}

    void AudioMixer::send_frames(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) {
        if (!sink_) return;
        const auto frame_size = sink_->frame_size();
        bytes::unique_binary mixed_output = bytes::make_unique_binary(frame_size);
        std::fill_n(reinterpret_cast<int16_t*>(mixed_output.get()), frame_size / sizeof(int16_t), 0);

        const auto num_sources = frames.size();
        for (size_t i = 0; i < frame_size / sizeof(int16_t); i++) {
            int32_t mixed_sample = 0;
            for (const auto& [fst, snd] : frames | std::views::values) {
                const auto source_samples = reinterpret_cast<const int16_t*>(fst.get());
                mixed_sample += source_samples[i];
            }

            // Audio normalization
            mixed_sample /= static_cast<int32_t>(num_sources);

            // Clipping to a 16-bit signed integer range
            const auto mixed_output_samples = reinterpret_cast<int16_t*>(mixed_output.get());
            mixed_output_samples[i] = static_cast<int16_t>(std::clamp(mixed_sample, -32768, 32767));
        }

        on_data(std::move(mixed_output));
    }
} // ntgcalls::io