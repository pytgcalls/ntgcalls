//
// Created by Laky64 on 26/10/24.
//

#pragma once
#include <cstdint>

namespace ntgcalls {

    struct RemoteSource {
        enum class State {
            Inactive,
            Suspended,
            Active
        };

        uint32_t ssrc = 0;
        State state = State::Inactive;
        StreamManager::Device device{};
    };

} // ntgcalls
