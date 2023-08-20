//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "../enums.hpp"

namespace wrtc {

  class RTCOnDataEvent {
  public:
    RTCOnDataEvent(binary &, uint16_t);

    uint8_t *audioData;
    uint16_t numberOfFrames;
    uint16_t sampleRate = 48000;
    uint8_t bitsPerSample = 16;
    uint8_t channelCount = 1;
  };

} // namespace wrtc
