//
// Created by Laky64 on 12/08/2023.
//

#pragma once

// i420 VIDEO CODEC SPECIFICATION
// Frame Time: 1000 / FPS ms
// Max FPS: 60
// Max Height: 1280
// Max Width: 1280
// FrameSize: A YUV frame size for a Width * Height resolution image,
// where Y (luminance) and UV (chrominance) components are combined with a 3:2 pixel ratio.


#include "base_streamer.hpp"

namespace ntgcalls {
    class VideoStreamer final : public BaseStreamer {
        std::shared_ptr<wrtc::RTCVideoSource> video;
        uint16_t w = 0, h = 0;
        uint8_t fps = 0;

        std::chrono::nanoseconds frameTime() override;

    public:
        VideoStreamer();

        ~VideoStreamer();

        wrtc::MediaStreamTrack *createTrack() override;

        void sendData(const wrtc::binary& sample) override;

        int64_t frameSize() override;

        void setConfig(uint16_t width, uint16_t height, uint8_t framesPerSecond);
    };
}

