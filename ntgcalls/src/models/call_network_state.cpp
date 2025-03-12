//
// Created by Laky64 on 03/10/24.
//

#include <ntgcalls/models/call_network_state.hpp>

namespace ntgcalls {
    NetworkInfo::NetworkInfo(const ConnectionState connectionState, const Kind kind) : state(connectionState), kind(kind) {}
} // ntgcalls