//
// Created by Laky64 on 15/08/2023.
//

#pragma once

namespace wrtc {
    typedef uint8_t *binary;

    enum RTCIceComponent {
        kRtp,
        kRtcp
    };

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
