//
// Created by Lauren on 15/04/25.
//

#pragma once

#include <utility>
#include <api/video/video_frame.h>

namespace wrtc::interfaces::media {

    struct VideoStreamingPartFrame {
        std::string endpoint_id;
        webrtc::VideoFrame frame;
        double pts = 0;
        int index = 0;

        VideoStreamingPartFrame(std::string endpoint_id, webrtc::VideoFrame frame, const double pts, const int index):
        endpoint_id(std::move(endpoint_id)), frame(std::move(frame)), pts(pts), index(index) {}
    };

} // wrtc::interfaces::media
