//
// Created by Lauren on 03/10/24.
//

#pragma once

namespace ntgcalls {

    class ConnectionInfo {
    public:
        enum class State {
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

        State state;
        Kind kind;

        ConnectionInfo(State state, Kind kind);
    };

} // ntgcalls
