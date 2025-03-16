//
// Created by Laky64 on 08/08/2023.
//

#include <wrtc/models/rtc_session_description.hpp>
#include <wrtc/exceptions.hpp>

namespace wrtc {
    Description::Description(const webrtc::SdpType type, std::string sdp): _type(type), _sdp(std::move(sdp)) {}

    webrtc::SdpType Description::type() const {
        return _type;
    }

    std::string Description::sdp() const {
        return _sdp;
    }

    std::string Description::SdpTypeToString(const webrtc::SdpType type) {
        switch (type) {
        case webrtc::SdpType::kOffer:
            return "offer";
        case webrtc::SdpType::kAnswer:
            return "answer";
        case webrtc::SdpType::kPrAnswer:
            return "pranswer";
        case webrtc::SdpType::kRollback:
            return "rollback";
        }
        throw std::runtime_error("Invalid sdp type");
    }
} // wrtc