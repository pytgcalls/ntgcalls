//
// Created by Laky64 on 19/08/2023.
//

#pragma once

#include <media/base/adapted_video_track_source.h>

namespace wrtc {

    class VideoTrackSource: public rtc::AdaptedVideoTrackSource {
    public:
        explicit VideoTrackSource(bool is_screencast = false, absl::optional<bool> needs_denoising = false);

        SourceState state() const override;

        bool remote() const override;

        bool is_screencast() const override;

        absl::optional<bool> needs_denoising() const override;

        void PushFrame(const webrtc::VideoFrame& frame);

    private:
        bool _is_screencast;
        absl::optional<bool> _needs_denoising;
    };

} // wrtc
