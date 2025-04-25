//
// Created by Laky64 on 15/04/25.
//

#include <wrtc/models/video_streaming_av_frame.hpp>

namespace wrtc {
    VideoStreamingAVFrame::VideoStreamingAVFrame() {
        frame = av_frame_alloc();
    }

    VideoStreamingAVFrame::VideoStreamingAVFrame(VideoStreamingAVFrame&& other) noexcept {
        frame = other.frame;
        other.frame = nullptr;
    }

    VideoStreamingAVFrame::~VideoStreamingAVFrame() {
        if (frame) {
            av_frame_free(&frame);
        }
    }

    AVFrame* VideoStreamingAVFrame::getFrame() const {
        return frame;
    }

    double VideoStreamingAVFrame::pts(const AVStream* stream, double& firstFramePts) const {
        const int64_t framePts = frame->pts;
        const double spf = av_q2d(stream->time_base);
        const double value = static_cast<double>(framePts) * spf;

        if (firstFramePts < 0.0) {
            firstFramePts = value;
        }

        return value - firstFramePts;
    }
} // wrtc