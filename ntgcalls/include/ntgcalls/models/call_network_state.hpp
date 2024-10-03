//
// Created by Laky64 on 03/10/24.
//

#pragma once

namespace ntgcalls {

    class CallNetworkState {
    public:
        enum class ConnectionState {
            Connecting = 1 << 0,
            Connected = 1 << 1,
            Failed = 1 << 2,
            Timeout = 1 << 3,
            Closed = 1 << 4
        };

        enum class Kind {
            Normal,
            Presentation
        };

        ConnectionState connectionState;
        Kind kind;

        CallNetworkState(ConnectionState connectionState, Kind kind);
    };

} // ntgcalls
