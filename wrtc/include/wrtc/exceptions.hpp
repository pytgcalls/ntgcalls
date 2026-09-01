//
// Created by Lauren on 08/08/23.
//

#pragma once

#include <exception>
#include <string>
#include <utility>
#include <api/jsep.h>
#include <api/rtc_error.h>

#define EX_GROUP(name)                                                            \
    class name: public wrtc::BaseRTCException {                                   \
    public:                                                                       \
        explicit name(std::string msg): wrtc::BaseRTCException(std::move(msg)) {} \
    };

#define EX_GROUP_INTERNAL(name) \
    EX_GROUP(name)

#define EX_GROUP_EXPORT(name) \
    EX_GROUP(name)

#define EX_DECLARE(name, base)                                  \
    class name final: public base {                             \
    public:                                                     \
        explicit name(std::string msg): base(std::move(msg)) {} \
    };

#define EX_DECLARE_INTERNAL(name, base) \
    EX_DECLARE(name, base)

namespace wrtc {

    class BaseRTCException: public std::exception {
    public:
        explicit BaseRTCException(std::string msg);

        [[nodiscard]] const char* what() const noexcept override;

    private:
        std::string msg_;
    };

    EX_GROUP_EXPORT(RTCException)
    EX_DECLARE(SdpParseException, RTCException)
    EX_DECLARE(TransportParseException, RTCException)

    RTCException wrap_rtc_error(const webrtc::RTCError& error);

    SdpParseException wrap_sdp_parse_error(const webrtc::SdpParseError& error);
} // wrtc
