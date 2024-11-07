//
// Created by Laky64 on 04/11/24.
//

#ifndef IS_ANDROID
#include <wrtc/video_factory/software/openh264/layer_config.hpp>

namespace openh264 {
    void LayerConfig::SetStreamState(const bool sendStream) {
        if (sendStream && !sending) {
            keyFrameRequest = true;
        }
        sending = sendStream;
    }
} // openh264
#endif