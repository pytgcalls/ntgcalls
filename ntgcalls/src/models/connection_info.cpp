//
// Created by Lauren on 03/10/24.
//

#include <ntgcalls/models/connection_info.hpp>

namespace ntgcalls {
    ConnectionInfo::ConnectionInfo(const State state, const Kind kind): state(state), kind(kind) {}
} // ntgcalls
