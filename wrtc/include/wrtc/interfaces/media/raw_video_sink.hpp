//
// Created by Laky64 on 26/10/24.
//

#pragma once
#include <api/video/video_frame.h>
#include <api/video/video_sink_interface.h>

namespace wrtc {

    class RawVideoSink final: public rtc::VideoSinkInterface<webrtc::VideoFrame> {
        std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)> callbackData;
        uint32_t ssrc = 0;

    public:
        void OnFrame(const webrtc::VideoFrame& frame) override;

        void setRemoteVideoSink(uint32_t ssrc, std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)> callback);
    };

} // wrtc
