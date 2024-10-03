//
// Created by Laky64 on 15/08/2023.
//

#pragma once

namespace wrtc {
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
        Closed,
    };

    enum class ConnectionState {
        Unknown,
        New,
        Connecting,
        Connected,
        Disconnected,
        Failed,
        Closed,
    };
}
