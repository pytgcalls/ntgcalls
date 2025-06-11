//
// Created by Laky64 on 15/04/25.
//

#pragma once
#include <api/make_ref_counted.h>
#include <api/video/video_frame_buffer.h>

namespace wrtc {

    webrtc::scoped_refptr<webrtc::VideoFrameBuffer> extractCVPixelBuffer(void *data);

} // wrtc
