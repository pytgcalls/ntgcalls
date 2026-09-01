//
// Created by Lauren on 22/08/23.
//

#pragma once

#include <optional>
#include <string>

namespace ntgcalls::media {
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
        MediaSource media_source;
        bool keep_open;

        BaseMediaDescription(std::string input, const MediaSource media_source, const bool keep_open): input(std::move(input)), media_source(media_source), keep_open(keep_open) {}

        virtual ~BaseMediaDescription() = default;
    };

    class AudioDescription final: public BaseMediaDescription {
    public:
        uint32_t sample_rate;
        uint8_t channel_count;

        AudioDescription(const MediaSource media_source, const uint32_t sample_rate, const uint8_t channel_count, const std::string& input, const bool keep_open):
        BaseMediaDescription(input, media_source, keep_open), sample_rate(sample_rate), channel_count(channel_count) {}
    };

    inline bool operator==(const AudioDescription& lhs, const AudioDescription& rhs) {
        return lhs.sample_rate == rhs.sample_rate &&
               lhs.channel_count == rhs.channel_count &&
               lhs.input == rhs.input &&
               lhs.media_source == rhs.media_source;
    }

    class VideoDescription final: public BaseMediaDescription {
    public:
        int16_t width, height;
        uint8_t fps;

        VideoDescription(const MediaSource media_source, const int16_t width, const int16_t height, const uint8_t fps, const std::string& input, const bool keep_open):
        BaseMediaDescription(input, media_source, keep_open), width(width), height(height), fps(fps) {}
    };

    inline bool operator==(const VideoDescription& lhs, const VideoDescription& rhs) {
        return lhs.width == rhs.width &&
               lhs.height == rhs.height &&
               lhs.fps == rhs.fps &&
               lhs.input == rhs.input &&
               lhs.media_source == rhs.media_source;
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
