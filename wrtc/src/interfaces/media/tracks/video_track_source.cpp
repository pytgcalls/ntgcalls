//
// Created by Lauren on 19/08/23.
//

#include <wrtc/interfaces/media/tracks/video_track_source.hpp>

namespace wrtc::interfaces::media::tracks {

    VideoTrackSource::VideoTrackSource(const bool is_screencast, const std::optional<bool> needs_denoising):
    is_screencast_(is_screencast), needs_denoising_(needs_denoising) {}

    webrtc::MediaSourceInterface::SourceState VideoTrackSource::state() const {
        return kLive;
    }

    bool VideoTrackSource::remote() const {
        return false;
    }

    bool VideoTrackSource::is_screencast() const {
        return is_screencast_;
    }

    std::optional<bool> VideoTrackSource::needs_denoising() const {
        return needs_denoising_;
    }

    void VideoTrackSource::push_frame(const webrtc::VideoFrame& frame) {
        OnFrame(frame);
    }

} // wrtc::interfaces::media::tracks
