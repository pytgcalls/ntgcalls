//
// Created by Lauren on 04/09/24.
//

#include <wrtc/interfaces/media/local_video_adapter.hpp>

namespace wrtc::interfaces::media {
    LocalVideoAdapter::LocalVideoAdapter(): sink_(std::nullopt){}

    LocalVideoAdapter::~LocalVideoAdapter() {
        const webrtc::MutexLock lock(&lock_);
        sink_ = std::nullopt;
    }

    void LocalVideoAdapter::OnFrame(const webrtc::VideoFrame& frame) {
        const webrtc::MutexLock lock(&lock_);
        if(sink_.has_value()) {
            sink_.value().sink->OnFrame(frame);
        }
    }

    void LocalVideoAdapter::AddOrUpdateSink(VideoSinkInterface* sink, const webrtc::VideoSinkWants& wants){
        const webrtc::MutexLock lock(&lock_);
        RTC_DCHECK(!sink_ || !sink_.has_value());
        sink_ = SinkPair(sink, wants);
    }

    void LocalVideoAdapter::RemoveSink(VideoSinkInterface* sink) {
        const webrtc::MutexLock lock(&lock_);
        if (sink_.has_value() && sink_.value().sink == sink) {
            sink_ = std::nullopt;
        }
    }
} // wrtc::interfaces::media