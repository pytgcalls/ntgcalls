//
// Created by Laky64 on 22/08/2023.
//

#pragma once

#include <string>
#include <map>

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

        BaseMediaDescription(std::string input, InputMode inputMode): input(input), inputMode(inputMode) {}
    };

    class AudioDescription: public BaseMediaDescription {
    public:
        uint32_t sampleRate;
        uint8_t bitsPerSample, channelCount;

        AudioDescription(InputMode inputMode, uint32_t sampleRate, uint8_t bitsPerSample, uint8_t channelCount, std::string input):
                sampleRate(sampleRate), bitsPerSample(bitsPerSample), channelCount(channelCount), BaseMediaDescription(input, inputMode) {};
    };

    class VideoDescription: public BaseMediaDescription {
    public:
        uint16_t width, height;
        uint8_t fps;

        VideoDescription(InputMode inputMode, uint16_t width, uint16_t height, uint8_t fps, std::string input):
                width(width), height(height), fps(fps), BaseMediaDescription(input, inputMode) {};
    };

    class MediaDescription {
    public:
        std::optional<AudioDescription> audio;
        std::optional<VideoDescription> video;

        MediaDescription(std::optional<AudioDescription> audio, std::optional<VideoDescription> video) {
            this->audio = audio;
            this->video = video;
        }
    };

} // ntgcalls
