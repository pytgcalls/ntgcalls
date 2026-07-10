//
// Created by Lauren on 12/08/23.
//

#pragma once

#include <ntgcalls/media/base_streamer.hpp>
#include <ntgcalls/media/video_sink.hpp>
#include <wrtc/interfaces/media/rtc_video_source.hpp>

namespace ntgcalls::media {
    class VideoStreamer final : public VideoSink, public BaseStreamer {
        std::unique_ptr<wrtc::interfaces::media::RTCVideoSource> video_;

    public:
        VideoStreamer();

        ~VideoStreamer() override;

        webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> createTrack() override;

        void sendData(uint8_t* sample, size_t size, wrtc::models::FrameData additional_data) override;
    };
} // ntgcalls::media
