//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <webrtc/rtc_base/ref_counted_object.h>
#include <api/video/i420_buffer.h>

#include "rtc_video_track_source.hpp"
#include "../media_stream_track.hpp"
#include "../../models/i420_image_data.hpp"
#include "../../models/rtc_on_data_event.hpp"

namespace wrtc {
    class RTCVideoSource {
    public:
        RTCVideoSource();

        RTCVideoSource(bool is_screencast, absl::optional<bool> needs_denoising);

        MediaStreamTrack *createTrack();

        void OnFrame(I420ImageData data);

    private:
        rtc::scoped_refptr<RTCVideoTrackSource> _source;
    };
}
