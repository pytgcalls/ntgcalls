//
// Created by Laky64 on 19/08/2023.
//

#include "video_track_source.hpp"

namespace wrtc {

    VideoTrackSource::VideoTrackSource(bool is_screencast, absl::optional<bool> needs_denoising) {
        _is_screencast = is_screencast;
        _needs_denoising = needs_denoising;
    }

    webrtc::MediaSourceInterface::SourceState VideoTrackSource::state() const {
        return webrtc::MediaSourceInterface::SourceState::kLive;
    }

    bool VideoTrackSource::remote() const {
        return false;
    }

    bool VideoTrackSource::is_screencast() const {
        return _is_screencast;
    }

    absl::optional<bool> VideoTrackSource::needs_denoising() const {
        return _needs_denoising;
    }

    void VideoTrackSource::PushFrame(const webrtc::VideoFrame &frame) {
        this->OnFrame(frame);
    }

} // wrtc