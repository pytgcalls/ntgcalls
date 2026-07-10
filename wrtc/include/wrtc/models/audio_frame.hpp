//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <cstddef>
#include <cstdint>

namespace wrtc::models {

    class AudioFrame {
    public:
        uint32_t ssrc;
        const int16_t* data = nullptr;
        size_t size = 0;
        int sample_rate = 0;
        size_t channels = 0;

        explicit AudioFrame(uint32_t ssrc);
    };

} // wrtc::models
