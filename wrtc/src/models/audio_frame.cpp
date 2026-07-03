//
// Created by Lauren on 07/10/24.
//

#include <wrtc/models/audio_frame.hpp>

namespace wrtc::models {
    AudioFrame::AudioFrame(const uint32_t ssrc): ssrc(ssrc) {}
} // wrtc::models