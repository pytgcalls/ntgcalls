//
// Created by Lauren on 26/10/24.
//

#include <libyuv.h>
#include <ntgcalls/media/video_receiver.hpp>

namespace ntgcalls::media {
    VideoReceiver::~VideoReceiver() {
        const std::lock_guard lock(mutex_);
        sink_ = nullptr;
        frame_callback_ = nullptr;
    }

    std::weak_ptr<wrtc::interfaces::media::RemoteVideoSink> VideoReceiver::remote_sink() {
        return sink_;
    }

    void VideoReceiver::on_frame(const std::function<void(uint32_t, bytes::unique_binary, size_t, wrtc::models::FrameData)>& callback) {
        frame_callback_ = callback;
    }

    void VideoReceiver::open() {
        sink_ = std::make_shared<wrtc::interfaces::media::RemoteVideoSink>([this](const uint32_t ssrc, const std::unique_ptr<webrtc::VideoFrame>& frame) {
            if (!description_) {
                return;
            }
            if (const auto sink = weakSink_.lock(); !sink) {
                return;
            }
            const std::lock_guard lock(mutex_);
            uint16_t new_width, new_height;
            if (description_->width <= 0) {
                new_width = static_cast<int16_t>(frame->width());
            } else {
                new_width = description_->width;
            }
            if (description_->height <= 0) {
                new_height = static_cast<int16_t>(frame->height());
            } else {
                new_height = description_->height;
            }
            const auto y_scaled_size = new_width * new_height;
            const auto uv_scaled_size = y_scaled_size / 4;
            const auto total_size = y_scaled_size + uv_scaled_size * 2;
            auto yuv = bytes::make_unique_binary(total_size);
            const auto buffer = frame->video_frame_buffer()->ToI420();
            const auto width = buffer->width();
            const auto height = buffer->height();
            const auto y_scaled_plane = std::make_unique<uint8_t[]>(y_scaled_size);
            const auto u_scaled_plane = std::make_unique<uint8_t[]>(uv_scaled_size);
            const auto v_scaled_plane = std::make_unique<uint8_t[]>(uv_scaled_size);

            I420Scale(
                buffer->DataY(), buffer->StrideY(),
                buffer->DataU(), buffer->StrideU(),
                buffer->DataV(), buffer->StrideV(),
                width, height,
                y_scaled_plane.get(), new_width,
                u_scaled_plane.get(), new_width / 2,
                v_scaled_plane.get(), new_width / 2,
                new_width, new_height,
                libyuv::kFilterBox
            );

            std::memcpy(yuv.get(), y_scaled_plane.get(), y_scaled_size);
            std::memcpy(yuv.get() + y_scaled_size, u_scaled_plane.get(), uv_scaled_size);
            std::memcpy(yuv.get() + y_scaled_size + uv_scaled_size, v_scaled_plane.get(), uv_scaled_size);

            (void) frame_callback_(ssrc, std::move(yuv), total_size, {
                frame->timestamp_us(),
                frame->rotation(),
                new_width,
                new_height
            });
        });
        weakSink_ = sink_;
    }
} // ntgcalls::media