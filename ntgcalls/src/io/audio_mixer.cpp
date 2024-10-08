//
// Created by Laky64 on 07/10/24.
//

#include <ntgcalls/io/audio_mixer.hpp>

namespace ntgcalls {
    AudioMixer::AudioMixer(BaseSink* sink): sink(sink) {}

    AudioMixer::~AudioMixer() {
        sink = nullptr;
    }

    void AudioMixer::sendFrames(const std::map<uint32_t, bytes::unique_binary>& frames) {
        bytes::unique_binary mixedOutput = bytes::make_unique_binary(sink->frameSize());
        std::fill_n(reinterpret_cast<int16_t*>(mixedOutput.get()), sink->frameSize() / sizeof(int16_t), 0);

        const auto numSources = frames.size();
        for (size_t i = 0; i < sink->frameSize() / sizeof(int16_t); ++i) {
            int32_t mixed_sample = 0;
            // ReSharper disable once CppUseElementsView
            for (const auto& [fst, snd] : frames) {
                const auto source_samples = reinterpret_cast<const int16_t*>(snd.get());
                mixed_sample += source_samples[i];
            }

            // Audio normalization
            mixed_sample /= numSources;

            // Clipping to 16-bit signed integer range
            const auto mixedOutputSamples = reinterpret_cast<int16_t*>(mixedOutput.get());
            mixedOutputSamples[i] = static_cast<int16_t>(std::clamp(mixed_sample, INT16_MIN, INT16_MAX));
        }

        onData(std::move(mixedOutput));
    }
} // ntgcalls