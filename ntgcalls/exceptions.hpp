//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <exception>
#include <string>

namespace ntgcalls {
    class ConnectionError: public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };
    class InvalidParams: public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };
    class RTMPNeeded: public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };
    class OSError: public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };
}
