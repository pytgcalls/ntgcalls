//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <string>
#include <api/jsep.h>

namespace wrtc {

  class RTCSessionDescriptionInit {
  public:
    RTCSessionDescriptionInit();

    RTCSessionDescriptionInit(webrtc::SdpType type, std::string sdp);

    static RTCSessionDescriptionInit Wrap(webrtc::SessionDescriptionInterface *);

    webrtc::SdpType type;
    std::string sdp;
  };

} //namespace wrtc
