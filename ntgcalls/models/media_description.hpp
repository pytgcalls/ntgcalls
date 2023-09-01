//
// Created by Laky64 on 22/08/2023.
//

#pragma once

#include <string>
#include <map>

namespace ntgcalls {
    class BaseMediaDescription {
    public:
        enum class Encoder {
            Raw,
            Shell,
            FFmpeg
        };

        std::string input;
        Encoder encoder;

        BaseMediaDescription(std::string input, Encoder encoder): input(input), encoder(encoder) {}
    };

    class RawAudioDescription: public BaseMediaDescription {
    protected:
        RawAudioDescription(uint16_t sampleRate, uint8_t bitsPerSample, uint8_t channelCount, std::string input, Encoder encoder):
                sampleRate(sampleRate), bitsPerSample(bitsPerSample), channelCount(channelCount), BaseMediaDescription(input, encoder) {};

    public:
        uint16_t sampleRate;
        uint8_t bitsPerSample, channelCount;

        RawAudioDescription(uint16_t sampleRate, uint8_t bitsPerSample, uint8_t channelCount, std::string input):
                RawAudioDescription(sampleRate, bitsPerSample, channelCount, input, Encoder::Raw) {};
    };

    class ShellAudioDescription: public RawAudioDescription {
    public:
        ShellAudioDescription(uint16_t sampleRate, uint8_t bitsPerSample, uint8_t channelCount, std::string command):
                RawAudioDescription(sampleRate, bitsPerSample, channelCount, command, Encoder::Shell) {};
    };

    class RawVideoDescription: public BaseMediaDescription {
    protected:
        RawVideoDescription(uint16_t width, uint16_t height, uint8_t fps, std::string input, Encoder encoder):
                width(width), height(height), fps(fps), BaseMediaDescription(input, encoder) {};

    public:
        uint16_t width, height;
        uint8_t fps;

        RawVideoDescription(uint16_t width, uint16_t height, uint8_t fps, std::string input):
                RawVideoDescription(width, height, fps, input, Encoder::Raw){};
    };

    class ShellVideoDescription: public RawVideoDescription {
    public:
        ShellVideoDescription(uint16_t width, uint16_t height, uint8_t channelCount, std::string command):
                RawVideoDescription(width, height, channelCount, command, Encoder::Shell) {};
    };

    class MediaDescription {
    public:
        std::optional<RawAudioDescription> audio;
        std::optional<RawVideoDescription> video;

        MediaDescription(std::optional<RawAudioDescription> audio, std::optional<RawVideoDescription> video) {
            this->audio = audio;
            this->video = video;
        }
    };

} // ntgcalls
