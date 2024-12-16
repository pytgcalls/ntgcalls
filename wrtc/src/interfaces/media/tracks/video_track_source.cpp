//
// Created by Laky64 on 19/08/2023.
//

#include <wrtc/interfaces/media/tracks/video_track_source.hpp>

namespace wrtc {

    VideoTrackSource::VideoTrackSource(const bool is_screencast, const std::optional<bool> needs_denoising) {
        _is_screencast = is_screencast;
        _needs_denoising = needs_denoising;
    }

    webrtc::MediaSourceInterface::SourceState VideoTrackSource::state() const {
        return kLive;
    }

    bool VideoTrackSource::remote() const {
        return false;
    }

    bool VideoTrackSource::is_screencast() const {
        return _is_screencast;
    }

    std::optional<bool> VideoTrackSource::needs_denoising() const {
        return _needs_denoising;
    }

    void VideoTrackSource::PushFrame(const webrtc::VideoFrame &frame) {
        OnFrame(frame);
    }

} // wrtc