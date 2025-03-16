//
// Created by Laky64 on 04/11/24.
//

#pragma once

#ifndef IS_ANDROID
#include <cstdint>

namespace openh264 {

    struct LayerConfig {
        int simulcastIdx = 0;
        int width = -1;
        int height = -1;
        bool sending = true;
        bool keyFrameRequest = false;
        float maxFrameRate = 0;
        uint32_t targetBps = 0;
        uint32_t maxBps = 0;
        bool frameDroppingOn = false;
        int keyFrameInterval = 0;
        int numTemporalLayers = 1;

        void SetStreamState(bool sendStream);
    };

} // openh264
#endif
