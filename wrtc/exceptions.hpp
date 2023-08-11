//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <exception>
#include "api/rtc_error.h"

namespace wrtc {

    class BaseRTCException: public std::exception {
    public:
        explicit BaseRTCException(std::string msg);
        [[nodiscard]] const char *what() const noexcept override;

    private:
        std::string _msg;
    };

    class RTCException: public BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    RTCException wrapRTCError(const webrtc::RTCError &error);
} // wrtc