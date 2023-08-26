//
// Created by Laky64 on 12/08/2023.
//

#pragma once

// i420 VIDEO CODEC SPECIFICATION
// Frame Time: 1000 / FPS ms
// Max FPS: 60
// Max Height: 1280
// Max Width: 1280
// FrameSize: 1280 * 720 * 1.5f


#include "base_streamer.hpp"

namespace ntgcalls {
    class VideoStreamer: public BaseStreamer {
    private:
        std::shared_ptr<wrtc::RTCVideoSource> video;
        uint16_t w = 0, h = 0;
        uint8_t fps = 0;

        std::chrono::nanoseconds frameTime() override;

    public:
        VideoStreamer();

        ~VideoStreamer();

        wrtc::MediaStreamTrack *createTrack() override;

        void sendData(wrtc::binary sample) override;

        uint64_t frameSize() override;

        void setConfig(uint16_t width, uint16_t height, uint8_t framesPerSecond);
    };
}

