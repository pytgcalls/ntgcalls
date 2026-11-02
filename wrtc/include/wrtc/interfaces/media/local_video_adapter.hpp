//
// Created by Lauren on 04/09/24.
//

#pragma once
#include <api/video/video_frame.h>
#include <api/video/video_source_interface.h>
#include <rtc_base/synchronization/mutex.h>

namespace wrtc::interfaces::media {
    class LocalVideoAdapter final: public webrtc::VideoSinkInterface<webrtc::VideoFrame>, public webrtc::VideoSourceInterface<webrtc::VideoFrame> {
        VideoSinkInterface* sink_;
        webrtc::Mutex lock_;

    public:
        LocalVideoAdapter();

        ~LocalVideoAdapter() override;

        void OnFrame(const webrtc::VideoFrame& frame) override;

        void AddOrUpdateSink(VideoSinkInterface* sink, const webrtc::VideoSinkWants& wants) override;

        void RemoveSink(VideoSinkInterface* sink) override;
    };
} // wrtc::interfaces::media
