//
// Created by Laky64 on 04/09/2024.
//

#include <wrtc/interfaces/media/local_video_adapter.hpp>

namespace wrtc {
    LocalVideoAdapter::LocalVideoAdapter(): _sink(std::nullopt){}

    LocalVideoAdapter::~LocalVideoAdapter() {
        webrtc::MutexLock lock(&lock_);
        _sink = std::nullopt;
    }

    void LocalVideoAdapter::OnFrame(const webrtc::VideoFrame& frame) {
        webrtc::MutexLock lock(&lock_);
        if(_sink.has_value()){
            _sink.value().sink->OnFrame(frame);
        }
    }

    void LocalVideoAdapter::AddOrUpdateSink(VideoSinkInterface* sink, const rtc::VideoSinkWants& wants){
        webrtc::MutexLock lock(&lock_);
        RTC_DCHECK(!sink || !_sink.has_value());
        _sink = SinkPair(sink, wants);
    }
} // wrtc