//
// Created by Lauren on 26/10/24.
//

#include <wrtc/interfaces/media/remote_video_sink.hpp>

namespace wrtc::interfaces::media {
    RemoteVideoSink::RemoteVideoSink(const std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)>& callback): frame_callback_(callback) {}

    RemoteVideoSink::~RemoteVideoSink() {
        frame_callback_ = nullptr;
    }

    void RemoteVideoSink::send_frame(const uint32_t ssrc, std::unique_ptr<webrtc::VideoFrame> frame) const {
        frame_callback_(ssrc, std::move(frame));
    }
} // wrtc::interfaces::media