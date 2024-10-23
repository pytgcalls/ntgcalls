//
// Created by Laky64 on 12/08/2023.
//

#include <ntgcalls/media/video_streamer.hpp>

namespace ntgcalls {
    VideoStreamer::VideoStreamer() {
        video = std::make_unique<wrtc::RTCVideoSource>();
    }

    VideoStreamer::~VideoStreamer() {
        video = nullptr;
    }

    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> VideoStreamer::createTrack() {
        return video->createTrack();
    }

    void VideoStreamer::sendData(uint8_t* sample, const wrtc::FrameData additionalData) {
        frames++;
        video->OnFrame(
            wrtc::i420ImageData(
                description->width,
                description->height,
                sample
            ),
            additionalData
        );
    }
}

