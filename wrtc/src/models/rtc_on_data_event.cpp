//
// Created by Lauren on 08/08/23.
// Derived from python-webrtc, Copyright 2022 Il`ya (Marshal) <https://github.com/MarshalX>, BSD-3-Clause.
//

#include <wrtc/models/rtc_on_data_event.hpp>

namespace wrtc::models {

    RTCOnDataEvent::RTCOnDataEvent(bytes::byte* data, const uint16_t length) {
        audio_data = data;
        number_of_frames = length;
    }

    RTCOnDataEvent::~RTCOnDataEvent() {
        audio_data = nullptr;
    }

} // namespace wrtc
