//
// Created by Laky64 on 08/08/2023.
//

#include "rtc_session_description_init.hpp"

namespace wrtc {
  RTCSessionDescriptionInit::RTCSessionDescriptionInit(const webrtc::SdpType type, std::string sdp) :
      type(type), sdp(std::move(sdp)) {}

  RTCSessionDescriptionInit RTCSessionDescriptionInit::Wrap(const webrtc::SessionDescriptionInterface *description) {
    std::string sdp;
    description->ToString(&sdp);

    return {description->GetType(), sdp};
  }

}
