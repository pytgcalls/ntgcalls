//
// Created by Lauren on 20/11/26.
//

#pragma once
#include <cstddef>
#include <cstdint>
#include <api/audio_codecs/audio_format.h>

namespace wrtc::audio_factory {
    class OpusPacket {
    public:
        static constexpr int kSampleRate = 48000;
        static constexpr int kFrameSamples = kSampleRate / 100;

        static int samples(const uint8_t* data, size_t size);

        static size_t frames(const uint8_t* data, size_t size);

        static size_t channels(const uint8_t* data, size_t size);

        static size_t channels(const webrtc::SdpAudioFormat& format);
    };
} // wrtc::audio_factory
