//
// Created by Laky64 on 12/08/2023.
//

#include "video_streamer.hpp"

namespace ntgcalls {
    VideoStreamer::VideoStreamer() {
        video = std::make_shared<wrtc::RTCVideoSource>();
    }

    VideoStreamer::~VideoStreamer() {
        fps = 0;
        w = 0;
        h = 0;
        video = nullptr;
    }

    std::chrono::nanoseconds VideoStreamer::frameTime() {
        return std::chrono::microseconds(static_cast<uint64_t>(1000.0 * 1000.0 / static_cast<double_t>(fps))); // ms
    }

    wrtc::MediaStreamTrack *VideoStreamer::createTrack() {
        return video->createTrack();
    }

    void VideoStreamer::sendData(const wrtc::binary& sample) {
        BaseStreamer::sendData(sample);
        video->OnFrame(
            wrtc::i420ImageData(
                w,
                h,
                sample
            )
        );
    }

    int64_t VideoStreamer::frameSize() {
        return llround(static_cast<float>(w * h) * 1.5f);
    }

    void VideoStreamer::setConfig(const uint16_t width, const uint16_t height, const uint8_t framesPerSecond) {
        clear();
        w = width;
        h = height;
        fps = framesPerSecond;
    }
}

