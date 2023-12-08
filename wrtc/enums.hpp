//
// Created by Laky64 on 15/08/2023.
//

#pragma once

#include <cstdint>
#include <memory>

namespace wrtc {
    typedef std::shared_ptr<uint8_t[]> binary;
    typedef uint32_t SSRC;
    typedef int32_t TgSSRC;

    enum class IceState: int {
        Unknown,
        New,
        Checking,
        Connected,
        Completed,
        Failed,
        Disconnected,
        Closed
    };

    enum class GatheringState: int {
        Unknown,
        New,
        InProgress,
        Complete
    };

    enum class SignalingState: int {
        Unknown,
        Stable,
        HaveLocalOffer,
        HaveRemoteOffer,
        HaveLocalPranswer,
        HaveRemotePranswer,
    };
}
