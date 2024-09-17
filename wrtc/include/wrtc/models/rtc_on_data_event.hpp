//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <cstdint>

#include <wrtc/utils/binary.hpp>

namespace wrtc {

  class RTCOnDataEvent {
  public:
    RTCOnDataEvent(uint8_t*, uint16_t);

    ~RTCOnDataEvent();

    uint8_t* audioData;
    uint16_t numberOfFrames;
    uint32_t sampleRate = 48000;
    uint8_t bitsPerSample = 16;
    uint8_t channelCount = 1;
  };

} // namespace wrtc
