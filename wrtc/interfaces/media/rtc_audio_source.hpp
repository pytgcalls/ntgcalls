//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <webrtc/api/media_stream_interface.h>
#include <webrtc/api/scoped_refptr.h>
#include <webrtc/pc/local_audio_source.h>
#include <webrtc/rtc_base/ref_counted_object.h>

#include "rtc_audio_track_source.hpp"
#include "../media_stream_track.hpp"

namespace wrtc {

  class RTCAudioSource {
  public:
    RTCAudioSource();

    MediaStreamTrack *createTrack();

    void OnData(RTCOnDataEvent &);

  private:
    rtc::scoped_refptr<RTCAudioTrackSource> _source;
  };

} // namespace wrtc
