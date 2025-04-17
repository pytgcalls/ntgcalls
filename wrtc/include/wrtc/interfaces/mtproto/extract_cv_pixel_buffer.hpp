//
// Created by Laky64 on 15/04/25.
//

#pragma once
#include <api/scoped_refptr.h>
#include <api/video/video_frame_buffer.h>
extern "C" {
#include <libavutil/frame.h>
}

namespace wrtc {

    webrtc::scoped_refptr<webrtc::VideoFrameBuffer> extractCVPixelBuffer(const AVFrame *frame);

} // wrtc
