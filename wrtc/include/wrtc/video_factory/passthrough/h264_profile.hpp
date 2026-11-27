//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <cstdint>
#include <string>

namespace wrtc::video_factory {
    class H264Profile {
    public:
        static bool forwardable(uint8_t profile_idc, bool progressive);

        static std::string name(uint8_t profile_idc);
    };
} // wrtc::video_factory
