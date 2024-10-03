//
// Created by Laky64 on 03/10/24.
//

#include <ntgcalls/models/call_network_state.hpp>

namespace ntgcalls {
    CallNetworkState::CallNetworkState(const ConnectionState connectionState, const Kind kind) : connectionState(connectionState), kind(kind) {}
} // ntgcalls