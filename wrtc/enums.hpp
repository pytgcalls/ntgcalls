//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <api/peer_connection_interface.h>

namespace wrtc {
    enum RTCIceComponent {
        kRtp,
        kRtcp
    };

    enum class State: int {
        New,
        Connecting,
        Connected,
        Disconnected,
        Failed,
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
