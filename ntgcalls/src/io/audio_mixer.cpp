//
// Created by Laky64 on 07/10/24.
//

#include <algorithm>
#include <ntgcalls/io/audio_mixer.hpp>

namespace ntgcalls {
    AudioMixer::AudioMixer(BaseSink* sink): AudioWriter(sink) {}

    void AudioMixer::sendFrames(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) {
        if (!sink) return;
        const auto frameSize = sink->frameSize();
        bytes::unique_binary mixedOutput = bytes::make_unique_binary(frameSize);
        std::fill_n(reinterpret_cast<int16_t*>(mixedOutput.get()), frameSize / sizeof(int16_t), 0);

        const auto numSources = frames.size();
        for (size_t i = 0; i < frameSize / sizeof(int16_t); i++) {
            int32_t mixedSample = 0;
            // ReSharper disable once CppUseElementsView
            for (const auto& [fst, snd] : frames) {
                const auto sourceSamples = reinterpret_cast<const int16_t*>(snd.first.get());
                mixedSample += sourceSamples[i];
            }

            // Audio normalization
            mixedSample /= static_cast<int32_t>(numSources);

            // Clipping to 16-bit signed integer range
            const auto mixedOutputSamples = reinterpret_cast<int16_t*>(mixedOutput.get());
            mixedOutputSamples[i] = static_cast<int16_t>(std::clamp(mixedSample, -32768, 32767));
        }

        onData(std::move(mixedOutput));
    }
} // ntgcalls