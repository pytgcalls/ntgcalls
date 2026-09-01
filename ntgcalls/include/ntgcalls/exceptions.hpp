//
// Created by Lauren on 12/08/23.
//

#pragma once
#include <wrtc/exceptions.hpp>

namespace ntgcalls {
    EX_GROUP(ConnectionException)
    EX_DECLARE(ConnectionNotFound, ConnectionException)
    EX_DECLARE(ConnectionError, ConnectionException)
    EX_DECLARE(CryptoError, ConnectionException)
    EX_DECLARE(TelegramServerError, ConnectionException)
    EX_DECLARE(RTCConnectionNeeded, ConnectionException)
    EX_DECLARE(InvalidParams, ConnectionException)

    EX_GROUP(SignalingException)
    EX_DECLARE(SignalingError, SignalingException)
    EX_DECLARE(SignalingUnsupported, SignalingException)

    EX_GROUP(MediaException)
    EX_DECLARE(FileError, MediaException)
    EX_DECLARE(FFmpegError, MediaException)
    EX_DECLARE(ShellError, MediaException)
    EX_DECLARE(MediaDeviceError, MediaException)

    EX_DECLARE(RTMPStreamingUnsupported, wrtc::RTCException)
    EX_DECLARE_INTERNAL(EOFError, wrtc::RTCException)
    EX_DECLARE_INTERNAL(NullPointer, wrtc::RTCException)
} // ntgcalls
