//
// Created by Laky64 on 26/10/24.
//

#include <libyuv.h>
#include <ntgcalls/media/video_receiver.hpp>

namespace ntgcalls {
    VideoReceiver::~VideoReceiver() {
        std::lock_guard lock(mutex);
        sink = nullptr;
        frameCallback = nullptr;
    }

    std::weak_ptr<wrtc::RemoteVideoSink> VideoReceiver::remoteSink() {
        return sink;
    }

    void VideoReceiver::onFrame(const std::function<void(uint32_t, bytes::unique_binary, size_t, wrtc::FrameData)>& callback) {
        frameCallback = callback;
    }

    void VideoReceiver::open() {
        sink = std::make_shared<wrtc::RemoteVideoSink>([this](const uint32_t ssrc, const std::unique_ptr<webrtc::VideoFrame>& frame) {
            if (!description) {
                return;
            }
            if (const auto sink = weakSink.lock(); !sink) {
                return;
            }
            std::lock_guard lock(mutex);
            uint16_t newWidth, newHeight;
            if (description->width <= 0) {
                newWidth = static_cast<int16_t>(frame->width());
            } else {
                newWidth = description->width;
            }
            if (description->height <= 0) {
                newHeight = static_cast<int16_t>(frame->height());
            } else {
                newHeight = description->height;
            }
            const auto yScaledSize = newWidth * newHeight;
            const auto uvScaledSize = yScaledSize / 4;
            const auto totalSize = yScaledSize + uvScaledSize * 2;
            auto yuv = bytes::make_unique_binary(totalSize);
            const auto buffer = frame->video_frame_buffer()->ToI420();
            const auto width = buffer->width();
            const auto height = buffer->height();
            const auto yScaledPlane = std::make_unique<uint8_t[]>(yScaledSize);
            const auto uScaledPlane = std::make_unique<uint8_t[]>(uvScaledSize);
            const auto vScaledPlane = std::make_unique<uint8_t[]>(uvScaledSize);

            I420Scale(
                buffer->DataY(), buffer->StrideY(),
                buffer->DataU(), buffer->StrideU(),
                buffer->DataV(), buffer->StrideV(),
                width, height,
                yScaledPlane.get(), newWidth,
                uScaledPlane.get(), newWidth / 2,
                vScaledPlane.get(), newWidth / 2,
                newWidth, newHeight,
                libyuv::kFilterBox
            );

            memcpy(yuv.get(), yScaledPlane.get(), yScaledSize);
            memcpy(yuv.get() + yScaledSize, uScaledPlane.get(), uvScaledSize);
            memcpy(yuv.get() + yScaledSize + uvScaledSize, vScaledPlane.get(), uvScaledSize);

            (void) frameCallback(ssrc, std::move(yuv), totalSize, {
                frame->timestamp_us(),
                frame->rotation(),
                newWidth,
                newHeight
            });
        });
        weakSink = sink;
    }
} // ntgcalls