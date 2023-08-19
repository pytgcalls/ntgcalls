//
// Created by Laky64 on 08/08/2023.
//

#include "create_session_description_observer.hpp"

namespace wrtc {
    void CreateSessionDescriptionObserver::OnSuccess(webrtc::SessionDescriptionInterface *description) {
        _onSuccess(Description::Wrap(description));
        delete description;
    }

    void CreateSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
        _onFailure(wrapRTCError(error));
    }
}
