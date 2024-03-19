//
// Created by Laky64 on 08/08/2023.
//

#include "rtc_session_description.hpp"
#include "wrtc/exceptions.hpp"

namespace wrtc {
    Description::Description(const SdpType type, std::string sdp): _type(type), _sdp(std::move(sdp)) {}

    Description::Description(const webrtc::SessionDescriptionInterface *description) {
        switch (description->GetType()) {
            case webrtc::SdpType::kOffer:
                _type = SdpType::Offer;
                break;
            case webrtc::SdpType::kPrAnswer:
                _type = SdpType::Pranswer;
                break;
            case webrtc::SdpType::kAnswer:
                _type = SdpType::Answer;
                break;
            case webrtc::SdpType::kRollback:
                _type = SdpType::Rollback;
                break;
        }
        description->ToString(&_sdp);
    }


    Description::SdpType Description::type() const {
        return _type;
    }

    std::string Description::sdp() const {
        return _sdp;
    }

    Description::SdpType Description::SdpTypeFromString(const std::string& type) {
        if (type == "offer") {
            return SdpType::Offer;
        }
        if (type == "answer") {
            return SdpType::Answer;
        }
        if (type == "pranswer") {
            return SdpType::Pranswer;
        }
        if (type == "rollback") {
            return SdpType::Rollback;
        }
        throw std::runtime_error("Invalid sdp type");
    }

    std::string Description::SdpTypeToString(const SdpType type) {
        switch (type) {
            case SdpType::Offer:
                return "offer";
            case SdpType::Answer:
                return "answer";
            case SdpType::Pranswer:
                return "pranswer";
            case SdpType::Rollback:
                return "rollback";
        }
        throw std::runtime_error("Invalid sdp type");
    }
} // wrtc