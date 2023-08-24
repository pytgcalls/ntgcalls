//
// Created by Laky64 on 15/08/2023.
//

#pragma once

#include <stdint.h>

namespace wrtc {
    typedef uint8_t *binary;
    typedef uint32_t SSRC;
    typedef int32_t TgSSRC;

    enum class IceState: int {
        New,
        Checking,
        Connected,
        Completed,
        Failed,
        Disconnected,
        Closed
    };

    enum class GatheringState: int {
        New,
        InProgress,
        Complete
    };

    enum class SignalingState: int {
        Stable,
        HaveLocalOffer,
        HaveRemoteOffer,
        HaveLocalPranswer,
        HaveRemotePranswer,
    };
}
