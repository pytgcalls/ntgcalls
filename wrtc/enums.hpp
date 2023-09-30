//
// Created by Laky64 on 15/08/2023.
//

#pragma once

#include <stdint.h>
#include <memory>

namespace wrtc {
    typedef std::shared_ptr<uint8_t[]> binary;
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
