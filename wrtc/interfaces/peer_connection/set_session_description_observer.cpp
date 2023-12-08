//
// Created by Laky64 on 08/08/2023.
//

#include "set_session_description_observer.hpp"

namespace wrtc {

    void SetSessionDescriptionObserver::OnSuccess() {
        _onSuccess();
    }

    void SetSessionDescriptionObserver::OnFailure(const webrtc::RTCError error) {
        RTCException rtc_exception = wrapRTCError(error);
        _onFailure(rtc_exception);
    }

} // namespace wrtc
