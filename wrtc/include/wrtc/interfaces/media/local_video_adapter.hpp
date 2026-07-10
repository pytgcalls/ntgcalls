//
// Created by Lauren on 04/09/24.
//

#pragma once
#include <media/base/video_source_base.h>

namespace wrtc::interfaces::media {
    class LocalVideoAdapter final : public webrtc::VideoSinkInterface<webrtc::VideoFrame>, public webrtc::VideoSourceBaseGuarded {
        std::optional<SinkPair> sink_;
        webrtc::Mutex lock_;

    public:
        LocalVideoAdapter();

        ~LocalVideoAdapter() override;

        void OnFrame(const webrtc::VideoFrame& frame) override;

        void AddOrUpdateSink(VideoSinkInterface* sink, const webrtc::VideoSinkWants& wants) override;

        void RemoveSink(VideoSinkInterface* sink) override;
    };
} // wrtc::interfaces::media
