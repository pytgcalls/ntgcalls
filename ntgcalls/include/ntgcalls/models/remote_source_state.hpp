//
// Created by Laky64 on 26/10/24.
//

#pragma once
#include <cstdint>

namespace ntgcalls {

    struct RemoteSource {
        uint32_t ssrc = 0;
        StreamManager::Status state = StreamManager::Status::Idling;
        StreamManager::Device device{};
    };

} // ntgcalls
