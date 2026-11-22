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
            Device = 1 << 3,
            Desktop = 1 << 4,
            External = 1 << 5
        };

        enum class Format {
            PCM,
            Ogg,
            I420,
            Matroska
        };

        std::string input;
        MediaSource media_source;
        bool keep_open;

        BaseMediaDescription(std::string input, const MediaSource media_source, const bool keep_open): input(std::move(input)), media_source(media_source), keep_open(keep_open) {}

        virtual ~BaseMediaDescription() = default;

        [[nodiscard]] virtual bool is_encoded() const = 0;
    };

    class AudioDescription final: public BaseMediaDescription {
    public:
        uint32_t sample_rate;
        uint8_t channel_count;
        std::optional<uint32_t> bitrate;
        std::optional<Format> format;
        std::optional<uint32_t> max_delay_ms;

        AudioDescription(const MediaSource media_source, const uint32_t sample_rate, const uint8_t channel_count, const std::string& input, const bool keep_open, const std::optional<uint32_t>& bitrate = std::nullopt, const std::optional<Format>& format = std::nullopt, const std::optional<uint32_t>& max_delay_ms = std::nullopt):
        BaseMediaDescription(input, media_source, keep_open), sample_rate(sample_rate), channel_count(channel_count), bitrate(bitrate), format(format), max_delay_ms(max_delay_ms) {}

        [[nodiscard]] bool is_encoded() const override {
            return media_source != MediaSource::Device && format.value_or(Format::Ogg) == Format::Ogg;
        }
    };

    inline bool operator==(const AudioDescription& lhs, const AudioDescription& rhs) {
        return lhs.sample_rate == rhs.sample_rate &&
               lhs.channel_count == rhs.channel_count &&
               lhs.bitrate == rhs.bitrate &&
               lhs.format == rhs.format &&
               lhs.max_delay_ms == rhs.max_delay_ms &&
               lhs.input == rhs.input &&
               lhs.media_source == rhs.media_source;
    }

    class VideoDescription final: public BaseMediaDescription {
    public:
        int16_t width, height;
        uint8_t fps;
        std::optional<Format> format;

        VideoDescription(const MediaSource media_source, const int16_t width, const int16_t height, const uint8_t fps, const std::string& input, const bool keep_open, const std::optional<Format>& format = std::nullopt):
        BaseMediaDescription(input, media_source, keep_open), width(width), height(height), fps(fps), format(format) {}

        [[nodiscard]] bool is_encoded() const override {
            return media_source != MediaSource::Device && media_source != MediaSource::Desktop && format.value_or(Format::Matroska) == Format::Matroska;
        }
    };

    inline bool operator==(const VideoDescription& lhs, const VideoDescription& rhs) {
        return lhs.width == rhs.width &&
               lhs.height == rhs.height &&
               lhs.fps == rhs.fps &&
               lhs.format == rhs.format &&
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
