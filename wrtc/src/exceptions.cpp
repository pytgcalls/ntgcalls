//
// Created by Lauren on 08/08/23.
//

#include <wrtc/exceptions.hpp>

namespace wrtc {
    const char* BaseRTCException::what() const noexcept {
        return msg_.c_str();
    }

    BaseRTCException::BaseRTCException(std::string msg): msg_(std::move(msg)) {}

    RTCException wrap_rtc_error(const webrtc::RTCError& error) {
        return RTCException{"[" + std::string(ToString(error.type())) + "] " + error.message()};
    }

    SdpParseException wrap_sdp_parse_error(const webrtc::SdpParseError& error) {
        if (error.line.empty()) {
            return SdpParseException{error.description};
        }
        return SdpParseException{"Line: " + error.line + ".  " + error.description};
    }
}
