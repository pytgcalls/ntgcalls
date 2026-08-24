//
// Created by Lauren on 26/10/24.
//

#include <wrtc/interfaces/media/raw_video_sink.hpp>

namespace wrtc::interfaces::media {
    RawVideoSink::~RawVideoSink() {
        callback_ = nullptr;
    }

    void RawVideoSink::OnFrame(const webrtc::VideoFrame& frame) {
        if (callback_) {
            callback_(ssrc_, std::make_unique<webrtc::VideoFrame>(frame));
        }
    }

    void RawVideoSink::set_remote_video_sink(const uint32_t ssrc, std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)> callback) {
        ssrc_ = ssrc;
        callback_ = std::move(callback);
    }
} // wrtc::interfaces::media
