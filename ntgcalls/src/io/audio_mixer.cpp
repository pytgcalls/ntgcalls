//
// Created by Lauren on 07/10/24.
//

#include <algorithm>
#include <ntgcalls/io/audio_mixer.hpp>

namespace ntgcalls::io {
    AudioMixer::AudioMixer(media::BaseSink* sink): BaseWriter(sink) {}

    void AudioMixer::send_frames(std::vector<wrtc::models::Frame> frames) {
        if (!sink_) return;
        const auto frame_size = sink_->frame_size();
        on_data(mix(frames, frame_size), frame_size);
    }

    bytes::unique_binary AudioMixer::mix(const std::vector<wrtc::models::Frame>& frames, const int64_t frame_size) {
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
} // ntgcalls::io
