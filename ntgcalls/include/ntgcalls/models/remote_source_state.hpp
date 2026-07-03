//
// Created by Lauren on 26/10/24.
//

#pragma once
#include <cstdint>

namespace ntgcalls {

    struct RemoteSource {
        uint32_t ssrc = 0;
        media::StreamManager::Status state = media::StreamManager::Status::Idling;
        media::StreamManager::Device device{};
    };

} // ntgcalls
