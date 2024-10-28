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

    void VideoReceiver::onFrame(const std::function<void(uint32_t, bytes::unique_binary, wrtc::FrameData)>& callback) {
        frameCallback = callback;
    }

    void VideoReceiver::open() {
        std::weak_ptr weakSink = sink;
        sink = std::make_shared<wrtc::RemoteVideoSink>([this, weakSink](const uint32_t ssrc, std::unique_ptr<webrtc::VideoFrame> frame) {
            if (!description) {
                return;
            }
            if (const auto sink = weakSink.lock(); !sink) {
                return;
            }
            std::lock_guard lock(mutex);
            const auto yScaledSize = description->width * description->height;
            const auto uvScaledSize = yScaledSize / 4;
            auto yuv = bytes::make_unique_binary(yScaledSize + uvScaledSize * 2);
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
                yScaledPlane.get(), description->width,
                uScaledPlane.get(), description->width / 2,
                vScaledPlane.get(), description->width / 2,
                description->width, description->height,
                libyuv::kFilterBox
            );

            memcpy(yuv.get(), yScaledPlane.get(), yScaledSize);
            memcpy(yuv.get() + yScaledSize, uScaledPlane.get(), uvScaledSize);
            memcpy(yuv.get() + yScaledSize + uvScaledSize, vScaledPlane.get(), uvScaledSize);

            (void) frameCallback(ssrc, std::move(yuv), {
                .rotation = frame->rotation()
            });
        });
    }
} // ntgcalls