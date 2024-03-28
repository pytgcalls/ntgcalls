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
            Unknown = 0,
            File = 1 << 0,
            Shell = 1 << 1,
            FFmpeg = 1 << 2,
            NoLatency = 1 << 3,
        };

        std::string input;
        InputMode inputMode;

        BaseMediaDescription(std::string  input, const InputMode inputMode): input(std::move(input)), inputMode(inputMode) {}
    };

    inline int operator&(const BaseMediaDescription::InputMode lhs, const int rhs) {
        return static_cast<int>(lhs) & rhs;
    }

    inline int operator|(const BaseMediaDescription::InputMode lhs, const BaseMediaDescription::InputMode rhs) {
        return static_cast<int>(lhs) | static_cast<int>(rhs);
    }

    inline int operator|(const BaseMediaDescription::InputMode lhs, const int rhs) {
        return static_cast<int>(lhs) | rhs;
    }

    inline BaseMediaDescription::InputMode operator|=(BaseMediaDescription::InputMode &lhs, BaseMediaDescription::InputMode rhs) {
        lhs = static_cast<BaseMediaDescription::InputMode>(static_cast<int>(lhs) | static_cast<int>(rhs));
        return lhs;
    }

    inline int operator&(const BaseMediaDescription::InputMode& lhs, const BaseMediaDescription::InputMode rhs){
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }

    inline int operator==(const int lhs, const BaseMediaDescription::InputMode& rhs){
        return lhs == static_cast<int>(rhs);
    }


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

        MediaDescription(const std::optional<AudioDescription>& audio, const std::optional<VideoDescription>& video):
                audio(audio), video(video) {}
    };
} // ntgcalls
