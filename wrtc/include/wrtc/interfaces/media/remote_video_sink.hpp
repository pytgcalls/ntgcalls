//
// Created by Laky64 on 26/10/24.
//

#pragma once
#include <functional>
#include <api/video/video_frame.h>
#include <wrtc/interfaces/media/remote_media_interface.hpp>

namespace wrtc {

    class RemoteVideoSink final: public RemoteMediaInterface, public std::enable_shared_from_this<RemoteVideoSink>{
        std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)> frameCallback;

    public:
        explicit RemoteVideoSink(const std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)>& callback);

        void sendFrame(uint32_t ssrc, std::unique_ptr<webrtc::VideoFrame> frame) const;
    };

} // wrtc
