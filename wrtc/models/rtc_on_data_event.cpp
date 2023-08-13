//
// Created by Laky64 on 08/08/2023.
//

#include "rtc_on_data_event.hpp"

namespace wrtc {

  RTCOnDataEvent::RTCOnDataEvent(binary &data, uint16_t length) {
    audioData = data;
    numberOfFrames = length;
  }

} // namespace wrtc
