//
// Created by Lauren on 04/09/24.
//

#include <wrtc/interfaces/media/local_video_adapter.hpp>

namespace wrtc::interfaces::media {
    LocalVideoAdapter::LocalVideoAdapter(): sink_(nullptr) {}

    LocalVideoAdapter::~LocalVideoAdapter() {
        const webrtc::MutexLock lock(&lock_);
        sink_ = nullptr;
    }

    void LocalVideoAdapter::OnFrame(const webrtc::VideoFrame& frame) {
        const webrtc::MutexLock lock(&lock_);
        if (sink_) {
            sink_->OnFrame(frame);
        }
    }

    void LocalVideoAdapter::AddOrUpdateSink(VideoSinkInterface* sink, const webrtc::VideoSinkWants& wants) {
        const webrtc::MutexLock lock(&lock_);
        RTC_DCHECK(!sink_ || sink_ == sink);
        sink_ = sink;
    }

    void LocalVideoAdapter::RemoveSink(VideoSinkInterface* sink) {
        const webrtc::MutexLock lock(&lock_);
        if (sink_ == sink) {
            sink_ = nullptr;
        }
    }
} // wrtc::interfaces::media
