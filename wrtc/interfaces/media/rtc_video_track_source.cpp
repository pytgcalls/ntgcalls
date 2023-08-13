//
// Created by Laky64 on 12/08/2023.
//

#include "rtc_video_track_source.hpp"

namespace wrtc {
    RTCVideoTrackSource::~RTCVideoTrackSource() {
        PeerConnectionFactory::Release();
        _factory = nullptr;
    }

    RTCVideoTrackSource::RTCVideoTrackSource(bool is_screencast, absl::optional<bool> needs_denoising)
            : rtc::AdaptedVideoTrackSource(), _is_screencast(is_screencast), _needs_denoising(needs_denoising) {}

    RTCVideoTrackSource::RTCVideoTrackSource() : rtc::AdaptedVideoTrackSource(), _is_screencast(false) {}

    webrtc::MediaSourceInterface::SourceState RTCVideoTrackSource::state() const {
        return webrtc::MediaSourceInterface::SourceState::kLive;
    }

    bool RTCVideoTrackSource::remote() const {
        return false;
    }

    bool RTCVideoTrackSource::is_screencast() const {
        return _is_screencast;
    }

    absl::optional<bool> RTCVideoTrackSource::needs_denoising() const {
        return _needs_denoising;
    }

    void RTCVideoTrackSource::PushFrame(const webrtc::VideoFrame &frame) {
        this->OnFrame(frame);
    }
} // wrtc