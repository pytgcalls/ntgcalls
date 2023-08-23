//
// Created by Laky64 on 22/08/2023.
//

#pragma once

#include <string>
#include <map>

namespace ntgcalls {

    class FFmpegOptions {
    public:
        uint8_t streamId;

        FFmpegOptions(uint8_t streamId): streamId(streamId) {};
    };

    class AudioDescription {
    public:
        uint16_t sampleRate;
        uint8_t bitsPerSample, channelCount;
        std::string path;
        FFmpegOptions options;

        AudioDescription(uint16_t sampleRate, uint8_t bitsPerSample, uint8_t channelCount, std::string path, FFmpegOptions options):
            sampleRate(sampleRate), bitsPerSample(bitsPerSample), channelCount(channelCount), path(path), options(options) {};
    };

    class VideoDescription {
    public:
        uint16_t width, height;
        uint8_t fps;
        std::string path;
        FFmpegOptions options;

        VideoDescription(uint16_t width, uint16_t height, uint8_t fps, std::string path, FFmpegOptions options):
            width(width), height(height), fps(fps), path(path), options(options) {};
    };

    class MediaDescription {
    public:
        std::string encoder;
        std::optional<AudioDescription> audio;
        std::optional<VideoDescription> video;

        MediaDescription(std::string encoder, std::optional<AudioDescription> audio, std::optional<VideoDescription> video):
            encoder(encoder), audio(audio), video(video) {}
    };

} // ntgcalls
