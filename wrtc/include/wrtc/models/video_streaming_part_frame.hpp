//
// Created by Laky64 on 15/04/25.
//

#pragma once

#include <api/video/video_frame.h>

namespace wrtc {

    struct VideoStreamingPartFrame {
        std::string endpointId;
        webrtc::VideoFrame frame;
        double pts = 0;
        int index = 0;

        VideoStreamingPartFrame(const std::string& endpointId, const webrtc::VideoFrame& frame, const double pts, const int index) :
            endpointId(endpointId), frame(frame), pts(pts), index(index) {}
    };

} // wrtc
