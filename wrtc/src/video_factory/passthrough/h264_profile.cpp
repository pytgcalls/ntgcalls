//
// Created by Lauren on 27/11/26.
//

#include <wrtc/video_factory/passthrough/h264_profile.hpp>

namespace wrtc::video_factory {
    bool H264Profile::forwardable(const uint8_t profile_idc, const bool progressive) {
        return progressive && (profile_idc == 66 || profile_idc == 77 || profile_idc == 100);
    }

    std::string H264Profile::name(const uint8_t profile_idc) {
        switch (profile_idc) {
        case 44:
            return "CAVLC 4:4:4 Intra";
        case 66:
            return "Baseline";
        case 77:
            return "Main";
        case 88:
            return "Extended";
        case 100:
            return "High";
        case 110:
            return "High 10";
        case 122:
            return "High 4:2:2";
        case 244:
            return "High 4:4:4";
        default:
            return std::to_string(profile_idc);
        }
    }
} // wrtc::video_factory
