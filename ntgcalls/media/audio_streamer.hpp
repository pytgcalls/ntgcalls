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

#include "base_streamer.hpp"

namespace ntgcalls {
    class AudioStreamer final : public BaseStreamer {
        std::unique_ptr<wrtc::RTCAudioSource> audio;
        uint8_t bps = 0, channels = 0;
        uint32_t rate = 0;

        std::chrono::nanoseconds frameTime() override;

    public:
        AudioStreamer();

        ~AudioStreamer();

        wrtc::MediaStreamTrack *createTrack() override;

        void sendData(const bytes::shared_binary& sample, int64_t absolute_capture_timestamp_ms) override;

        int64_t frameSize() override;

        void setConfig(uint32_t sampleRate, uint8_t bitsPerSample, uint8_t channelCount);
    };
}
