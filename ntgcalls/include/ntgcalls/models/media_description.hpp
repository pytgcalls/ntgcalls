//
// Created by Laky64 on 22/08/2023.
//

#pragma once

#include <optional>
#include <string>

namespace ntgcalls {
    class BaseMediaDescription {
    public:
        enum class MediaSource {
            Unknown = 0,
            File = 1 << 0,
            Shell = 1 << 1,
            FFmpeg = 1 << 2,
            Device = 1 << 3,
            Desktop = 1 << 4,
            External = 1 << 5
        };

        std::string input;
        MediaSource mediaSource;

        BaseMediaDescription(std::string input, const MediaSource mediaSource): input(std::move(input)), mediaSource(mediaSource) {}

        virtual ~BaseMediaDescription() = default;
    };

    inline int operator&(const BaseMediaDescription::MediaSource lhs, const int rhs) {
        return static_cast<int>(lhs) & rhs;
    }

    inline int operator|(const BaseMediaDescription::MediaSource lhs, const BaseMediaDescription::MediaSource rhs) {
        return static_cast<int>(lhs) | static_cast<int>(rhs);
    }

    inline int operator|(const BaseMediaDescription::MediaSource lhs, const int rhs) {
        return static_cast<int>(lhs) | rhs;
    }

    inline BaseMediaDescription::MediaSource operator|=(BaseMediaDescription::MediaSource &lhs, BaseMediaDescription::MediaSource rhs) {
        lhs = static_cast<BaseMediaDescription::MediaSource>(static_cast<int>(lhs) | static_cast<int>(rhs));
        return lhs;
    }

    inline int operator&(const BaseMediaDescription::MediaSource& lhs, const BaseMediaDescription::MediaSource rhs){
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }

    inline int operator==(const int lhs, const BaseMediaDescription::MediaSource& rhs){
        return lhs == static_cast<int>(rhs);
    }

    class AudioDescription final : public BaseMediaDescription {
    public:
        uint32_t sampleRate;
        uint8_t channelCount;

        AudioDescription(const MediaSource mediaSource, const uint32_t sampleRate, const uint8_t channelCount, const std::string& input):
                BaseMediaDescription(input, mediaSource), sampleRate(sampleRate), channelCount(channelCount) {}
    };

    inline bool operator==(const AudioDescription& lhs, const AudioDescription& rhs) {
        return lhs.sampleRate == rhs.sampleRate &&
            lhs.channelCount == rhs.channelCount &&
                lhs.input == rhs.input &&
                    lhs.mediaSource == rhs.mediaSource;
    }

    class VideoDescription final : public BaseMediaDescription {
    public:
        int16_t width, height;
        uint8_t fps;

        VideoDescription(const MediaSource mediaSource, const int16_t width, const int16_t height, const uint8_t fps, const std::string& input):
                BaseMediaDescription(input, mediaSource), width(width), height(height), fps(fps) {}
    };

    inline bool operator==(const VideoDescription& lhs, const VideoDescription& rhs) {
        return lhs.width == rhs.width &&
            lhs.height == rhs.height &&
                lhs.fps == rhs.fps &&
                    lhs.input == rhs.input &&
                        lhs.mediaSource == rhs.mediaSource;
    }

    class MediaDescription {
    public:
        std::optional<AudioDescription> microphone, speaker;
        std::optional<VideoDescription> camera, screen;

        explicit MediaDescription(
            const std::optional<AudioDescription>& microphone = std::nullopt,
            const std::optional<AudioDescription>& speaker = std::nullopt,
            const std::optional<VideoDescription>& camera = std::nullopt,
            const std::optional<VideoDescription>& screen = std::nullopt
        ): microphone(microphone), speaker(speaker), camera(camera), screen(screen) {}
    };
} // ntgcalls
