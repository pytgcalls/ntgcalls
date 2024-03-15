//
// Created by Laky64 on 08/08/2023.
//

#include "rtc_session_description.hpp"

#include "wrtc/exceptions.hpp"

namespace wrtc {
    Description::Description(const Type type, const std::string &sdp) {
        webrtc::SdpType newType = {};
        switch (type) {
            case Type::Offer:
                newType = webrtc::SdpType::kOffer;
                break;
            case Type::Answer:
                newType = webrtc::SdpType::kAnswer;
                break;
            case Type::Pranswer:
                newType = webrtc::SdpType::kPrAnswer;
                break;
            case Type::Rollback:
                newType = webrtc::SdpType::kRollback;
                break;
        }
        *this = Description(RTCSessionDescriptionInit(newType, sdp));
    }

    Description::Description(const RTCSessionDescriptionInit &rtcSessionDescriptionInit) {
        webrtc::SdpParseError error;
        auto description = CreateSessionDescription(rtcSessionDescriptionInit.type, rtcSessionDescriptionInit.sdp, &error);
        if (!description) {
            throw wrapSdpParseError(error);
        }

        _description = std::move(description);
    }

    Description::Type Description::getType() const
    {
        switch (_description->GetType()) {
            case webrtc::SdpType::kOffer:
                return Type::Offer;
            case webrtc::SdpType::kPrAnswer:
                return Type::Pranswer;
            case webrtc::SdpType::kAnswer:
                return Type::Answer;
            case webrtc::SdpType::kRollback:
                return Type::Rollback;
        }
        return {};
    }

    std::string Description::getSdp() const
    {
        std::string sdp;
        _description->ToString(&sdp);
        return sdp;
    }

    Description::Type Description::parseType(const std::string& type) {
        if (type == "offer") {
            return Type::Offer;
        }
        if (type == "answer") {
            return Type::Answer;
        }
        if (type == "pranswer") {
            return Type::Pranswer;
        }
        if (type == "rollback") {
            return Type::Rollback;
        }
    }

    Description::operator webrtc::SessionDescriptionInterface *() const
    {
        return CreateSessionDescription(_description->GetType(), this->getSdp()).release();
    }

    Description Description::Wrap(const webrtc::SessionDescriptionInterface *description) {
        return Description(RTCSessionDescriptionInit::Wrap(description));
    }
} // wrtc