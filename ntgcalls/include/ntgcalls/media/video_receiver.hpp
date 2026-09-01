//
// Created by Lauren on 26/10/24.
//

#pragma once
#include <ntgcalls/media/base_receiver.hpp>
#include <ntgcalls/media/video_sink.hpp>
#include <wrtc/interfaces/media/remote_video_sink.hpp>
#include <wrtc/models/frame_data.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace ntgcalls::media {

    class VideoReceiver final: public VideoSink, public BaseReceiver {
        std::shared_ptr<wrtc::interfaces::media::RemoteVideoSink> sink_;
        wrtc::utils::synchronized_callback<void(uint32_t, bytes::unique_binary, size_t, wrtc::models::FrameData)> frame_callback_;

    public:
        ~VideoReceiver() override;

        void on_frame(const std::function<void(uint32_t, bytes::unique_binary, size_t, wrtc::models::FrameData)>& callback);

        std::weak_ptr<wrtc::interfaces::media::RemoteVideoSink> remote_sink();

        void open() override;
    };

} // ntgcalls::media
