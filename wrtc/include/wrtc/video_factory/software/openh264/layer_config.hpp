//
// Created by Lauren on 04/11/24.
//

#pragma once

#ifndef IS_ANDROID
#include <cstdint>

namespace openh264 {

    struct LayerConfig {
        int simulcast_idx = 0;
        int width = -1;
        int height = -1;
        bool sending = true;
        bool key_frame_request = false;
        float max_frame_rate = 0;
        uint32_t target_bps = 0;
        uint32_t max_bps = 0;
        bool frame_dropping_on = false;
        int key_frame_interval = 0;
        int num_temporal_layers = 1;

        void set_stream_state(bool send_stream);
    };

} // openh264
#endif
