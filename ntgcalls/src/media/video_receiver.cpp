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
        removed_callback_ = nullptr;
    }

    std::weak_ptr<wrtc::interfaces::media::RemoteVideoSink> VideoReceiver::remote_sink() {
        return sink_;
    }

    void VideoReceiver::on_frame(const std::function<void(uint32_t, bytes::binary, wrtc::models::FrameData)>& callback) {
        frame_callback_ = callback;
    }

    void VideoReceiver::on_source_removed(const std::function<void(uint32_t)>& callback) {
        removed_callback_ = callback;
    }

    void VideoReceiver::set_encoded(const bool encoded) const {
        sink_->set_encoded(encoded);
    }

    void VideoReceiver::open() {
        sink_ = std::make_shared<wrtc::interfaces::media::RemoteVideoSink>(
            [this](const uint32_t ssrc, const std::unique_ptr<webrtc::VideoFrame>& frame) {
                const auto description = this->description();
                if (!description) {
                    return;
                }
                if (const auto sink = weak_sink_.lock(); !sink) {
                    return;
                }
                const std::lock_guard lock(mutex_);
                uint16_t new_width, new_height;
                const bool native = description->media_source != VideoDescription::MediaSource::External;
                if (native || description->width <= 0) {
                    new_width = static_cast<int16_t>(frame->width());
                } else {
                    new_width = description->width;
                }
                if (native || description->height <= 0) {
                    new_height = static_cast<int16_t>(frame->height());
                } else {
                    new_height = description->height;
                }
                const auto chroma_width = (new_width + 1) / 2;
                const auto chroma_height = (new_height + 1) / 2;
                const auto y_scaled_size = new_width * new_height;
                const auto uv_scaled_size = chroma_width * chroma_height;
                const auto total_size = y_scaled_size + uv_scaled_size * 2;
                bytes::binary yuv(total_size);
                const auto buffer = frame->video_frame_buffer()->ToI420();
                I420Scale(
                    buffer->DataY(), buffer->StrideY(),
                    buffer->DataU(), buffer->StrideU(),
                    buffer->DataV(), buffer->StrideV(),
                    buffer->width(), buffer->height(),
                    yuv.data(), new_width,
                    yuv.data() + y_scaled_size, chroma_width,
                    yuv.data() + y_scaled_size + uv_scaled_size, chroma_width,
                    new_width, new_height,
                    libyuv::kFilterBox
                );

                (void) frame_callback_(
                    ssrc,
                    std::move(yuv),
                    {
                        frame->timestamp_us(),
                        frame->rotation(),
                        new_width,
                        new_height,
                        wrtc::models::Codec::I420,
                        true,
                    }
                );
            },
            [this](const uint32_t ssrc, const wrtc::utils::encoded::EncodedFrame& frame) {
                if (!description()) {
                    return;
                }
                if (const auto sink = weak_sink_.lock(); !sink) {
                    return;
                }
                const std::lock_guard lock(mutex_);
                (void) frame_callback_(ssrc, std::move(frame.payload), frame.frame_data);
            },
            [this](const uint32_t ssrc) {
                (void) removed_callback_(ssrc);
            }
        );
        weak_sink_ = sink_;
    }
} // ntgcalls::media
