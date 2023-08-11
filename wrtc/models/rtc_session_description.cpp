//
// Created by Laky64 on 08/08/2023.
//

#include "rtc_session_description.hpp"

namespace wrtc {
    Description::Description(const RTCSessionDescriptionInit &init) {
        webrtc::SdpParseError error;
        auto description = webrtc::CreateSessionDescription(init.type, init.sdp, &error);
        if (!description) {
            throw std::runtime_error(error.description);
        }

        _description = std::move(description);
    }

    webrtc::SdpType Description::getType() {
        return _description->GetType();
    }

    std::string Description::getSdp() {
        std::string sdp;
        _description->ToString(&sdp);
        return sdp;
    }

    Description::operator webrtc::SessionDescriptionInterface *() {
        return webrtc::CreateSessionDescription(this->getType(), this->getSdp()).release();
    }

    Description Description::Wrap(webrtc::SessionDescriptionInterface *description) {
        return Description(RTCSessionDescriptionInit::Wrap(description));
    }
} // wrtc