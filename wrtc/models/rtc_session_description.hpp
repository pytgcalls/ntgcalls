//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <future>

#include "rtc_session_description_init.hpp"

namespace wrtc {

    class Description {
    public:
        enum class Type {Offer, Answer, Pranswer, Rollback };

        Description(Type type, const std::string &sdp);

        explicit Description(const RTCSessionDescriptionInit &rtcSessionDescriptionInit);

        static Description Wrap(const webrtc::SessionDescriptionInterface *);

        explicit operator webrtc::SessionDescriptionInterface *() const;

        [[nodiscard]] Type getType() const;

        [[nodiscard]] std::string getSdp() const;

        static Type parseType(const std::string &type);

    private:
        std::unique_ptr<webrtc::SessionDescriptionInterface> _description;
    };

} // namespace wrtc
