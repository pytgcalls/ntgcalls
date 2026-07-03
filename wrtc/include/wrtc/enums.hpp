//
// Created by Lauren on 15/08/23.
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
        HaveLocalPrAnswer,
        HaveRemotePrAnswer,
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

    enum class ConnectionMode {
        None,
        Rtc,
        Stream,
        Rtmp,
    };
}
