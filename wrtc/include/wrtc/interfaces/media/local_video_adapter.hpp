//
// Created by Laky64 on 04/09/2024.
//

#pragma once
#include <media/base/video_source_base.h>

namespace wrtc {
    class LocalVideoAdapter final : public webrtc::VideoSinkInterface<webrtc::VideoFrame>, public webrtc::VideoSourceBaseGuarded {
        std::optional<SinkPair> _sink;
        webrtc::Mutex lock_;

    public:
        LocalVideoAdapter();

        ~LocalVideoAdapter() override;

        void OnFrame(const webrtc::VideoFrame& frame) override;

        void AddOrUpdateSink(VideoSinkInterface* sink, const webrtc::VideoSinkWants& wants) override;
    };
} // wrtc
