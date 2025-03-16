//
// Created by Laky64 on 08/08/2023.
//

#include <wrtc/interfaces/peer_connection/set_session_description_observer.hpp>

#include <wrtc/exceptions.hpp>

namespace wrtc {

    void SetSessionDescriptionObserver::OnSetRemoteDescriptionComplete(const webrtc::RTCError error) {
        if (error.ok()) {
            _onSuccess();
        } else {
            _onFailure(std::make_exception_ptr(wrapRTCError(error)));
        }
    }

    void SetSessionDescriptionObserver::OnSetLocalDescriptionComplete(const webrtc::RTCError error) {
        if (error.ok()) {
            _onSuccess();
        } else {
            _onFailure(std::make_exception_ptr(wrapRTCError(error)));
        }
    }
} // namespace wrtc
