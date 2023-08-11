//
// Created by Laky64 on 08/08/2023.
//

#include "exceptions.hpp"

namespace wrtc {
    [[nodiscard]] const char *BaseRTCException::what() const noexcept {
        return _msg.c_str();
    }

    BaseRTCException::BaseRTCException(std::string msg) : _msg(std::move(msg)) {}

    RTCException wrapRTCError(const webrtc::RTCError &error) {
        std::string msg;
        return RTCException{msg + "[" + ToString(error.type()) + "] " + error.message()};
    }
}