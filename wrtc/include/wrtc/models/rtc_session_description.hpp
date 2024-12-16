//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <api/jsep.h>

namespace wrtc {

    class Description {
    public:
        Description(webrtc::SdpType type, std::string sdp);

        [[nodiscard]] webrtc::SdpType type() const;

        [[nodiscard]] std::string sdp() const;

        static std::string SdpTypeToString(webrtc::SdpType type);

    private:
        webrtc::SdpType _type;
        std::string _sdp;
    };
} // namespace wrtc
