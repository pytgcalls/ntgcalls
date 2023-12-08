//
// Created by Laky64 on 08/08/2023.
//

#include "create_session_description_observer.hpp"

namespace wrtc {
    void CreateSessionDescriptionObserver::OnSuccess(webrtc::SessionDescriptionInterface *description) {
        _onSuccess(Description::Wrap(description));
        delete description;
    }

    void CreateSessionDescriptionObserver::OnFailure(const webrtc::RTCError error) {
        RTCException rtc_exception = wrapRTCError(error);
        _onFailure(rtc_exception);
    }
}
