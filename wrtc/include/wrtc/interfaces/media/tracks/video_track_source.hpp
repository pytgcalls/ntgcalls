//
// Created by Lauren on 19/08/23.
//

#pragma once

#include <media/base/adapted_video_track_source.h>

namespace wrtc::interfaces::media::tracks {

    class VideoTrackSource: public webrtc::AdaptedVideoTrackSource {
    public:
        explicit VideoTrackSource(bool is_screencast = false, std::optional<bool> needs_denoising = false);

        SourceState state() const override;

        bool remote() const override;

        bool is_screencast() const override;

        std::optional<bool> needs_denoising() const override;

        void push_frame(const webrtc::VideoFrame& frame);

    private:
        bool is_screencast_;
        std::optional<bool> needs_denoising_;
    };

} // wrtc::interfaces::media::tracks
