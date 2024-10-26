//
// Created by Laky64 on 26/10/24.
//

#include <wrtc/interfaces/media/remote_video_sink.hpp>

namespace wrtc {
    RemoteVideoSink::RemoteVideoSink(const std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)>& callback): frameCallback(callback) {}

    void RemoteVideoSink::sendFrame(const uint32_t ssrc, std::unique_ptr<webrtc::VideoFrame> frame) const {
        frameCallback(ssrc, std::move(frame));
    }
} // wrtc