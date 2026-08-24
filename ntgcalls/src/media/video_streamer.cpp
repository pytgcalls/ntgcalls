//
// Created by Lauren on 12/08/23.
//

#include <ntgcalls/media/video_streamer.hpp>

namespace ntgcalls::media {
    VideoStreamer::VideoStreamer() {
        video_ = std::make_unique<wrtc::interfaces::media::RTCVideoSource>();
    }

    VideoStreamer::~VideoStreamer() {
        video_ = nullptr;
    }

    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> VideoStreamer::createTrack() {
        return video_->create_track();
    }

    void VideoStreamer::sendData(uint8_t* sample, const size_t size, wrtc::models::FrameData additional_data) {
        frames_++;
        if (additional_data.width == 0) {
            additional_data.width = description_->width;
        }
        if (additional_data.height == 0) {
            additional_data.height = description_->height;
        }
        if (additional_data.width == 0 || additional_data.height == 0 || size == 0) {
            return;
        }
        video_->on_frame(
            wrtc::models::I420ImageData(
                additional_data.width,
                additional_data.height,
                sample,
                size
            ),
            additional_data
        );
    }
} // ntgcalls::media
