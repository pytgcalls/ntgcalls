//
// Created by Lauren on 06/11/24.
//

#pragma once

#ifndef IS_ANDROID
#include <api/video/color_space.h>

extern "C" {
    #include <libavcodec/avcodec.h>
}

namespace openh264 {

    webrtc::ColorSpace extract_h264_color_space(const AVCodecContext* codec);

} // openh264
#endif