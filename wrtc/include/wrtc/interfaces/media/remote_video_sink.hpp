//
// Created by Lauren on 26/10/24.
//

#pragma once
#include <atomic>
#include <wrtc/utils/encoded/encoded_frame.hpp>
#include <functional>
#include <api/video/video_frame.h>
#include <wrtc/interfaces/media/remote_media_interface.hpp>

namespace wrtc::interfaces::media {

    class RemoteVideoSink final: public RemoteMediaInterface, public std::enable_shared_from_this<RemoteVideoSink> {
        std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)> frame_callback_;
        std::function<void(uint32_t, utils::encoded::EncodedFrame)> encoded_callback_;
        std::function<void(uint32_t)> removed_callback_;
        std::atomic_bool encoded_ = false;

    public:
        RemoteVideoSink(const std::function<void(uint32_t, std::unique_ptr<webrtc::VideoFrame>)>& callback, const std::function<void(uint32_t, utils::encoded::EncodedFrame)>& encoded_callback, const std::function<void(uint32_t)>& removed_callback);

        ~RemoteVideoSink() override;

        void send_frame(uint32_t ssrc, std::unique_ptr<webrtc::VideoFrame> frame) const;

        void send_encoded_frame(uint32_t ssrc, utils::encoded::EncodedFrame frame) const;

        void remove_source(uint32_t ssrc) const;

        void set_encoded(bool encoded);

        [[nodiscard]] bool encoded() const;
    };

} // wrtc::interfaces::media
