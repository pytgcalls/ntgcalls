//
// Created by Laky64 on 26/10/24.
//

#include <wrtc/interfaces/media/raw_video_sink.hpp>

namespace wrtc {
    void RawVideoSink::OnFrame(const webrtc::VideoFrame& frame) {
        if (callbackData) {
            callbackData(ssrc, std::make_unique<webrtc::VideoFrame>(frame));
        }
    }

    void RawVideoSink::setRemoteVideoSink(const uint32_t ssrc, std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)> callback) {
        callbackData = std::move(callback);
        this->ssrc = ssrc;
    }
} // wrtc