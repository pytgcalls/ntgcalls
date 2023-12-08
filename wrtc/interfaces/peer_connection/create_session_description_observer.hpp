//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include "../peer_connection.hpp"

namespace webrtc { class RTCError; }

namespace wrtc {

    class CreateSessionDescriptionObserver : public webrtc::CreateSessionDescriptionObserver {
    public:
        CreateSessionDescriptionObserver(const std::function<void(Description)>& onSuccess,
                                         const std::function<void(const std::exception&)>& onFailure) :
                                         _onSuccess(onSuccess), _onFailure(onFailure) {}

        void OnSuccess(webrtc::SessionDescriptionInterface *) override;

        void OnFailure(webrtc::RTCError) override;

    private:
        std::function<void(Description)> _onSuccess;
        std::function<void(const std::exception&)> _onFailure;
    };

} // namespace wrtc
