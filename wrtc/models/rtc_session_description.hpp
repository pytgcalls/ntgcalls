//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <future>

#include "rtc_session_description_init.hpp"
#include "../exceptions.hpp"

namespace wrtc {

    class Description {
    public:
        enum class Type {Offer, Answer, Pranswer, Rollback };

        Description(Type type, const std::string &sdp);

        explicit Description(const RTCSessionDescriptionInit &rtcSessionDescriptionInit);

        static Description Wrap(webrtc::SessionDescriptionInterface *);

        explicit operator webrtc::SessionDescriptionInterface *();

        Type getType();

        std::string getSdp();

    private:
        std::unique_ptr<webrtc::SessionDescriptionInterface> _description;
    };

} // namespace wrtc
