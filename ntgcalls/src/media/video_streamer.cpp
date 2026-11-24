//
// Created by Lauren on 12/08/23.
//

#include <ntgcalls/media/video_streamer.hpp>

namespace ntgcalls::media {
    VideoStreamer::VideoStreamer() {
        video_ = std::make_unique<wrtc::interfaces::media::RTCVideoSource>();
        encoded_ = std::make_shared<wrtc::utils::encoded::EncodedQueue>();
    }

    VideoStreamer::~VideoStreamer() {
        video_ = nullptr;
    }

    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> VideoStreamer::create_track() {
        return video_->create_track();
    }

    void VideoStreamer::send_data(uint8_t* sample, const size_t size, wrtc::models::FrameData additional_data) {
        const auto description = this->description();
        if (!description) {
            return;
        }
        frames_++;
        if (additional_data.width == 0) {
            additional_data.width = description->width;
        }
        if (additional_data.height == 0) {
            additional_data.height = description->height;
        }
        if (additional_data.width == 0 || additional_data.height == 0 || size == 0) {
            return;
        }
        if (description->is_encoded()) {
            encoded_->push({{sample, sample + size}, additional_data});
            video_->on_placeholder(additional_data);
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

    std::shared_ptr<wrtc::utils::encoded::EncodedQueue> VideoStreamer::encoded_queue() const {
        return encoded_;
    }
} // ntgcalls::media
