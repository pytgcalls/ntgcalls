//
// Created by Laky64 on 08/08/2023.
//

#include <wrtc/models/rtc_on_data_event.hpp>

namespace wrtc {

  RTCOnDataEvent::RTCOnDataEvent(uint8_t* data, const uint16_t length) {
    audioData = data;
    numberOfFrames = length;
  }

  RTCOnDataEvent::~RTCOnDataEvent() {
    audioData = nullptr;
  }

} // namespace wrtc
