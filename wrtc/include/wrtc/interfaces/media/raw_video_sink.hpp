//
// Created by Lauren on 26/10/24.
//

#pragma once
#include <api/video/video_frame.h>
#include <api/video/video_sink_interface.h>

namespace wrtc::interfaces::media {

    class RawVideoSink final: public webrtc::VideoSinkInterface<webrtc::VideoFrame> {
        std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)> callback_;
        uint32_t ssrc_ = 0;

    public:
        ~RawVideoSink() override;

        void OnFrame(const webrtc::VideoFrame& frame) override;

        void set_remote_video_sink(uint32_t ssrc, std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)> callback);
    };

} // wrtc::interfaces::media
