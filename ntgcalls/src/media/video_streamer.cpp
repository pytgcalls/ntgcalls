//
// Created by Lauren on 12/08/23.
//

#include <algorithm>
#include <ntgcalls/media/video_streamer.hpp>
#include <rtc_base/time_utils.h>

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
        ++frames_;
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
            const bool shown = within_frame_rate(description->fps);
            const bool rate_limited = description->fps > 0 && source_frame_rate() > description->fps + 0.5;
            encoded_->push({{sample, sample + size}, additional_data, 0, shown, rate_limited});
            if (shown) {
                video_->on_placeholder(fit(additional_data, description->width, description->height));
            }
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

    bool VideoStreamer::within_frame_rate(const uint8_t fps) {
        if (fps == 0) {
            return true;
        }
        const int64_t interval = 1000000 / fps;
        const int64_t now = webrtc::TimeMicros();
        if (next_shown_us_ >= 0 && now < next_shown_us_ - interval / 4) {
            return false;
        }
        next_shown_us_ = next_shown_us_ >= 0 && now - next_shown_us_ < interval ? next_shown_us_ + interval : now + interval;
        return true;
    }

    wrtc::models::FrameData VideoStreamer::fit(wrtc::models::FrameData frame, const int16_t width, const int16_t height) {
        if (width <= 0 || height <= 0 || (frame.width <= width && frame.height <= height)) {
            return frame;
        }
        const double scale = std::min(static_cast<double>(width) / frame.width, static_cast<double>(height) / frame.height);
        frame.width = std::max<uint16_t>(2, static_cast<uint16_t>(static_cast<uint16_t>(frame.width * scale) & ~1));
        frame.height = std::max<uint16_t>(2, static_cast<uint16_t>(static_cast<uint16_t>(frame.height * scale) & ~1));
        return frame;
    }

    std::shared_ptr<wrtc::utils::encoded::EncodedQueue> VideoStreamer::encoded_queue() const {
        return encoded_;
    }
} // ntgcalls::media
