//
// Created by Lauren on 15/04/25.
//

#pragma once
#include <api/make_ref_counted.h>
#include <api/video/video_frame_buffer.h>

namespace wrtc::interfaces::mtproto {

    webrtc::scoped_refptr<webrtc::VideoFrameBuffer> extract_cv_pixel_buffer(void* data);

} // wrtc::interfaces::mtproto
