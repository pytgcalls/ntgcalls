//
// Created by Laky64 on 22/08/2023.
//

#pragma once

#include <optional>
#include <string>
#include <utility>

namespace ntgcalls {
    class BaseMediaDescription {
    public:
        enum class InputMode {
            File,
            Shell,
            FFmpeg
        };

        std::string input;
        InputMode inputMode;

        BaseMediaDescription(std::string  input, const InputMode inputMode): input(std::move(input)), inputMode(inputMode) {}
    };

    class AudioDescription: public BaseMediaDescription {
    public:
        uint32_t sampleRate;
        uint8_t bitsPerSample, channelCount;

        AudioDescription(const InputMode inputMode, const uint32_t sampleRate, const uint8_t bitsPerSample, const uint8_t channelCount, const std::string& input):
                BaseMediaDescription(input, inputMode), sampleRate(sampleRate), bitsPerSample(bitsPerSample), channelCount(channelCount) {};
    };

    class VideoDescription: public BaseMediaDescription {
    public:
        uint16_t width, height;
        uint8_t fps;

        VideoDescription(const InputMode inputMode, const uint16_t width, const uint16_t height, const uint8_t fps, const std::string& input):
                BaseMediaDescription(input, inputMode), width(width), height(height), fps(fps) {}
    };

    class MediaDescription {
    public:
        std::optional<AudioDescription> audio;
        std::optional<VideoDescription> video;

        MediaDescription(const std::optional<AudioDescription>& audio, const std::optional<VideoDescription>& video) {
            this->audio = audio;
            this->video = video;
        }
    };

} // ntgcalls
