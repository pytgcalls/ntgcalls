//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <wrtc/interfaces/peer_connection.hpp>

namespace webrtc { class RTCError; }

namespace wrtc {

  class SetSessionDescriptionObserver : public webrtc::SetRemoteDescriptionObserverInterface, public webrtc::SetLocalDescriptionObserverInterface {
  public:
    SetSessionDescriptionObserver(
      const std::function<void()>& onSuccess,
      const std::function<void(const std::exception_ptr&)>& onFailure) :
        _onSuccess(onSuccess), _onFailure(onFailure) {}

    void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;

    void OnSetLocalDescriptionComplete(webrtc::RTCError error) override;

  private:
    std::function<void()> _onSuccess = nullptr;
    std::function<void(const std::exception_ptr&)> _onFailure = nullptr;
  };

} // namespace wrtc
