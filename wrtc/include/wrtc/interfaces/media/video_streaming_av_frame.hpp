//
// Created by Lauren on 15/04/25.
//

#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

namespace wrtc::interfaces::media {

    class VideoStreamingAVFrame {
        AVFrame* frame_ = nullptr;

    public:
        VideoStreamingAVFrame();

        VideoStreamingAVFrame(VideoStreamingAVFrame &&other) noexcept;

        ~VideoStreamingAVFrame();

        [[nodiscard]] AVFrame* get_frame() const;

        double pts(const AVStream* stream, double &first_frame_pts) const;
    };

} // wrtc::interfaces::media
