//
// Created by Lauren on 08/08/23.
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
