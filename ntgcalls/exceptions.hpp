//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <wrtc/wrtc.hpp>

namespace ntgcalls {
    class ConnectionError final : public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    class ConnectionNotFound final : public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    class InvalidParams final : public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    class RTMPNeeded final : public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    class FileError final : public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    class FFmpegError final : public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    class ShellError final : public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    class InvalidUUID final : public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };

    class EOFError final : public wrtc::BaseRTCException {
        using BaseRTCException::BaseRTCException;
    };
}
