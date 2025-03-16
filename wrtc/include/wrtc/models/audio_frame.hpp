//
// Created by Laky64 on 07/10/24.
//

#pragma once
#include <cstdint>
#include <wrtc/utils/binary.hpp>

namespace wrtc {

    class AudioFrame {
    public:
        uint32_t ssrc;
        const int16_t* data = nullptr;
        size_t size = 0;
        int sampleRate = 0;
        size_t channels = 0;

        explicit AudioFrame(uint32_t ssrc);
    };

} // wrtc
