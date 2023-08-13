//
// Created by Laky64 on 12/08/2023.
//

#pragma once

// PCM16L AUDIO CODEC SPECIFICATION
// Frame Time: 10ms
// Max SampleRate: 48000
// Max BitsPerSample: 16
// Max Channels: 2
// FrameSize: ((48000 * 16) / 8 / 100)) * 2

#include <wrtc/wrtc.hpp>

#include "base_streamer.hpp"

namespace ntgcalls {
    class AudioStreamer: public BaseStreamer {
    private:
        std::shared_ptr<wrtc::RTCAudioSource> audio;
        uint8_t bps = 0, channels = 0;
        uint16_t rate = 0;

        uint64_t frameTime() override;

    public:
        AudioStreamer();

        ~AudioStreamer();

        wrtc::MediaStreamTrack *createTrack() override;

        void sendData(wrtc::binary sample) override;

        uint64_t frameSize() override;

        void setConfig(uint16_t sampleRate, uint8_t bitsPerSample, uint8_t channelCount);
    };
}
