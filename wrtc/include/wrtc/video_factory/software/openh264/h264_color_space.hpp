//
// Created by Laky64 on 06/11/24.
//

#pragma once

#ifndef IS_ANDROID
#include <api/video/color_space.h>

extern "C" {
    #include <libavcodec/avcodec.h>
}

namespace openh264 {

    webrtc::ColorSpace ExtractH264ColorSpace(const AVCodecContext* codec);

} // openh264
#endif