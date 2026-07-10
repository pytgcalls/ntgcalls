//
// Created by Lauren on 08/08/23.
//

#pragma once

#include <cstdint>
#include <wrtc/utils/binary.hpp>

namespace wrtc::models {

  class RTCOnDataEvent {
  public:
    bytes::byte* audio_data;
    uint16_t number_of_frames;
    uint32_t sample_rate = 48000;
    uint8_t bits_per_sample = 16;
    uint8_t channel_count = 1;

    RTCOnDataEvent(bytes::byte*, uint16_t);

    ~RTCOnDataEvent();
  };

} // namespace wrtc
