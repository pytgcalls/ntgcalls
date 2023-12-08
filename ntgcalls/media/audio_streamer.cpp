//
// Created by Laky64 on 12/08/2023.
//

#include "audio_streamer.hpp"

namespace ntgcalls {
    AudioStreamer::AudioStreamer() {
        audio = std::make_shared<wrtc::RTCAudioSource>();
    }

    AudioStreamer::~AudioStreamer() {
        bps = 0;
        rate = 0;
        channels = 0;
        audio = nullptr;
    }

    wrtc::MediaStreamTrack *AudioStreamer::createTrack() {
        return audio->createTrack();
    }

    std::chrono::nanoseconds AudioStreamer::frameTime() {
        return std::chrono::milliseconds(10); // ms
    }

    void AudioStreamer::sendData(const wrtc::binary& sample) {
        BaseStreamer::sendData(sample);
        auto event = wrtc::RTCOnDataEvent(sample, frameSize() / (2 * channels));
        event.channelCount = channels;
        event.sampleRate = rate;
        event.bitsPerSample = bps;
        audio->OnData(event);
    }

    int64_t AudioStreamer::frameSize() {
        return rate * bps / 8 / 100 * channels;
    }

    void AudioStreamer::setConfig(const uint32_t sampleRate, const uint8_t bitsPerSample, const uint8_t channelCount) {
        clear();
        bps = bitsPerSample;
        rate = sampleRate;
        channels = channelCount;
    }
}
