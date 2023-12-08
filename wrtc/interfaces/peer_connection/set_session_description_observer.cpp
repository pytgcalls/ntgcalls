//
// Created by Laky64 on 08/08/2023.
//

#include "set_session_description_observer.hpp"

namespace wrtc {

    void SetSessionDescriptionObserver::OnSuccess() {
        _onSuccess();
    }

    void SetSessionDescriptionObserver::OnFailure(const webrtc::RTCError error) {
        _onFailure(wrapRTCError(error));
    }

} // namespace wrtc
