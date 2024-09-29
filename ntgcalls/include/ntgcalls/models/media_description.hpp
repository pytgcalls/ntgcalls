//
// Created by Laky64 on 22/08/2023.
//

#pragma once

#include <optional>
#include <string>

namespace ntgcalls {
    class BaseMediaDescription {
    public:
        enum class InputMode {
            Unknown = 0,
            File = 1 << 0,
            Shell = 1 << 1,
            FFmpeg = 1 << 2,
            Device = 1 << 4,
        };

        std::string input;
        InputMode inputMode;

        BaseMediaDescription(std::string input, const InputMode inputMode): input(std::move(input)), inputMode(inputMode) {}

        virtual ~BaseMediaDescription() = default;
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

    class AudioDescription final : public BaseMediaDescription {
    public:
        uint32_t sampleRate;
        uint8_t bitsPerSample, channelCount;

        AudioDescription(const InputMode inputMode, const uint32_t sampleRate, const uint8_t bitsPerSample, const uint8_t channelCount, const std::string& input):
                BaseMediaDescription(input, inputMode), sampleRate(sampleRate), bitsPerSample(bitsPerSample), channelCount(channelCount) {}
    };

    inline bool operator==(const AudioDescription& lhs, const AudioDescription& rhs) {
        return lhs.sampleRate == rhs.sampleRate &&
            lhs.bitsPerSample == rhs.bitsPerSample &&
                lhs.channelCount == rhs.channelCount &&
                    lhs.input == rhs.input &&
                        lhs.inputMode == rhs.inputMode;
    }

    class VideoDescription final : public BaseMediaDescription {
    public:
        uint16_t width, height;
        uint8_t fps;

        VideoDescription(const InputMode inputMode, const uint16_t width, const uint16_t height, const uint8_t fps, const std::string& input):
                BaseMediaDescription(input, inputMode), width(width), height(height), fps(fps) {}
    };

    inline bool operator==(const VideoDescription& lhs, const VideoDescription& rhs) {
        return lhs.width == rhs.width &&
            lhs.height == rhs.height &&
                lhs.fps == rhs.fps &&
                    lhs.input == rhs.input &&
                        lhs.inputMode == rhs.inputMode;
    }

    class MediaDescription {
    public:
        std::optional<AudioDescription> microphone, speaker;
        std::optional<VideoDescription> camera, screen;

        MediaDescription(
            const std::optional<AudioDescription>& microphone,
            const std::optional<AudioDescription>& speaker,
            const std::optional<VideoDescription>& camera,
            const std::optional<VideoDescription>& screen
        ): microphone(microphone), speaker(speaker), camera(camera), screen(screen) {}
    };
} // ntgcalls
