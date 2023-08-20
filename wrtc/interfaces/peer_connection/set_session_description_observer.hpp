//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include "../peer_connection.hpp"

namespace webrtc { class RTCError; }

namespace wrtc {

  class SetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
  public:
    SetSessionDescriptionObserver(
        std::function<void()> onSuccess,
        std::function<void(RTCException)> onFailure) :
        _onSuccess(onSuccess), _onFailure(onFailure) {}

    void OnSuccess() override;

    void OnFailure(webrtc::RTCError) override;

  private:
    std::function<void()> _onSuccess = nullptr;
    std::function<void(RTCException)> _onFailure = nullptr;
  };

} // namespace wrtc
