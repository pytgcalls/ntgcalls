//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <api/jsep.h>

namespace wrtc {

    class Description {
    public:
        enum class SdpType {
            Offer,
            Answer,
            Pranswer,
            Rollback
        };

        Description(SdpType type, std::string sdp);

        explicit Description(webrtc::SdpType type, std::string sdp);

        [[nodiscard]] SdpType type() const;

        [[nodiscard]] std::string sdp() const;

        static std::string SdpTypeToString(SdpType type);

    private:
        SdpType _type;
        std::string _sdp;
    };
} // namespace wrtc
