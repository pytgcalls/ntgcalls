//
// Created by Laky64 on 22/03/2024.
//

#pragma once

#include "message.hpp"
#include "wrtc/models/rtc_session_description.hpp"

namespace signaling {

class RtcDescriptionMessage final : public Message  {
public:
    wrtc::Description::SdpType type;
    std::string sdp;

    [[nodiscard]] bytes::binary serialize() const override;

    static std::unique_ptr<RtcDescriptionMessage> deserialize(const bytes::binary& data);

};

} // signaling
