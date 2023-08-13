//
// Created by Laky64 on 13/08/2023.
//

#pragma once

#include "io/base_reader.hpp"


namespace ntgcalls {

    class BaseConfig {
    public:
        std::shared_ptr<BaseReader> reader;

        BaseConfig(std::shared_ptr<BaseReader> reader): reader(reader){};
    };

    class AudioConfig: public BaseConfig{
    public:
        uint16_t sampleRate;
        uint8_t bitsPerSample;
        uint8_t channelCount;

        AudioConfig(
                std::shared_ptr<BaseReader> reader,
                uint16_t sampleRate,
                uint8_t bitsPerSample,
                uint8_t channelCount
        ): BaseConfig(reader), sampleRate(sampleRate), bitsPerSample(bitsPerSample), channelCount(channelCount){}
    };

    class VideoConfig: public BaseConfig{
    public:
        uint16_t width;
        uint16_t height;
        uint8_t fps;

        VideoConfig(
                std::shared_ptr<BaseReader> reader,
                uint16_t width,
                uint16_t height,
                uint8_t fps
        ): BaseConfig(reader), width(width), height(height), fps(fps){}
    };

    struct StreamConfig {
        std::optional<AudioConfig> audioConfig;
        std::optional<VideoConfig> videoConfig;
        bool lipSync;
    };

} // ntgcalls
