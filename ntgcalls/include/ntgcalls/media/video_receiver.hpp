//
// Created by Laky64 on 26/10/24.
//

#pragma once
#include <ntgcalls/media/video_sink.hpp>
#include <ntgcalls/media/base_receiver.hpp>
#include <wrtc/interfaces/media/remote_video_sink.hpp>
#include <wrtc/models/frame_data.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/syncronized_callback.hpp>

namespace ntgcalls {

    class VideoReceiver final: public VideoSink, public BaseReceiver {
        std::shared_ptr<wrtc::RemoteVideoSink> sink;
        wrtc::synchronized_callback<uint32_t, bytes::unique_binary, size_t, wrtc::FrameData> frameCallback;

    public:
        ~VideoReceiver() override;

        void onFrame(const std::function<void(uint32_t, bytes::unique_binary, size_t, wrtc::FrameData)>& callback);

        std::weak_ptr<wrtc::RemoteVideoSink> remoteSink();

        void open() override;
    };

} // ntgcalls
