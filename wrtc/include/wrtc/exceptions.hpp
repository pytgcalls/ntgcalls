//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <../../deps/libcxx/include/exception>
#include <../../deps/libwebrtc/src/include/api/rtc_error.h>
#include "../../deps/libwebrtc/src/include/api/jsep.h"

namespace wrtc {

    class BaseRTCException: public std::exception {
    public:
        explicit BaseRTCException(std::string msg);

        [[nodiscard]] const char* what() const noexcept override;

    private:
        std::string _msg;
    };

    class RTCException final : public BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    class SdpParseException final : public BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    RTCException wrapRTCError(const webrtc::RTCError &error);

    SdpParseException wrapSdpParseError(const webrtc::SdpParseError &error);
} // wrtc