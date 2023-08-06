//
// Created by Laky64 on 03/08/2023.
//

#include "RTCAudioSource.hpp"

RTCAudioSource::RTCAudioSource(rtc::Description::Direction dir): RTCMediaSource(MediaStreamTrack::Type::Audio, dir) {}