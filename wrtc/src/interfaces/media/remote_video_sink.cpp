//
// Created by Lauren on 26/10/24.
//

#include <wrtc/interfaces/media/remote_video_sink.hpp>

namespace wrtc::interfaces::media {
    RemoteVideoSink::RemoteVideoSink(const std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)>& callback, const std::function<void(uint32_t, utils::encoded::EncodedFrame)>& encoded_callback): frame_callback_(callback), encoded_callback_(encoded_callback) {}

    RemoteVideoSink::~RemoteVideoSink() {
        frame_callback_ = nullptr;
        encoded_callback_ = nullptr;
    }

    void RemoteVideoSink::send_frame(const uint32_t ssrc, std::unique_ptr<webrtc::VideoFrame> frame) const {
        frame_callback_(ssrc, std::move(frame));
    }

    void RemoteVideoSink::send_encoded_frame(const uint32_t ssrc, utils::encoded::EncodedFrame frame) const {
        encoded_callback_(ssrc, std::move(frame));
    }

    void RemoteVideoSink::set_encoded(const bool encoded) {
        encoded_ = encoded;
    }

    bool RemoteVideoSink::encoded() const {
        return encoded_;
    }
} // wrtc::interfaces::media
