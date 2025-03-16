//
// Created by Laky64 on 03/10/24.
//

#pragma once

namespace ntgcalls {

    class NetworkInfo {
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

        ConnectionState state;
        Kind kind;

        NetworkInfo(ConnectionState connectionState, Kind kind);
    };

} // ntgcalls
