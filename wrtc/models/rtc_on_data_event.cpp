//
// Created by Laky64 on 08/08/2023.
//

#include "rtc_on_data_event.hpp"

namespace wrtc {

  RTCOnDataEvent::RTCOnDataEvent(std::string &data, uint16_t length) {
    audioData = reinterpret_cast<uint8_t *>(data.data());
    numberOfFrames = length;
  }

} // namespace wrtc
