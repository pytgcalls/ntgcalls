//
// Created by Lauren on 15/04/25.
//

#include <wrtc/interfaces/media/video_streaming_av_frame.hpp>

namespace wrtc::interfaces::media {
    VideoStreamingAVFrame::VideoStreamingAVFrame() {
        frame_ = av_frame_alloc();
    }

    VideoStreamingAVFrame::VideoStreamingAVFrame(VideoStreamingAVFrame&& other) noexcept {
        frame_ = other.frame_;
        other.frame_ = nullptr;
    }

    VideoStreamingAVFrame::~VideoStreamingAVFrame() {
        if (frame_) {
            av_frame_free(&frame_);
        }
    }

    AVFrame* VideoStreamingAVFrame::get_frame() const {
        return frame_;
    }

    double VideoStreamingAVFrame::pts(const AVStream* stream, double& first_frame_pts) const {
        const int64_t frame_pts = frame_->pts;
        const double spf = av_q2d(stream->time_base);
        const double value = static_cast<double>(frame_pts) * spf;

        if (first_frame_pts < 0.0) {
            first_frame_pts = value;
        }

        return value - first_frame_pts;
    }
} // wrtc::interfaces::media