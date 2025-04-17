//
// Created by Laky64 on 15/04/25.
//

#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

namespace wrtc {

    class VideoStreamingAVFrame {
        AVFrame *frame = nullptr;

    public:
        VideoStreamingAVFrame();

        VideoStreamingAVFrame(VideoStreamingAVFrame &&other) noexcept;

        ~VideoStreamingAVFrame();

        AVFrame *getFrame() const;

        double pts(const AVStream *stream, double &firstFramePts) const;
    };

} // wrtc
