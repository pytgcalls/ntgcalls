//
// Created by Laky-64 on 19/06/26.
//

#pragma once
#include <cstdint>
#include <map>

namespace telegram::e2e {
    struct SubChainState {
        int shortPollGeneration = 0;
        int waitingGeneration = 0;
        bool waitingActive = false;
        int64_t lastUpdate = 0;
        std::map<int, bytes::binary> waiting;
        bool shortPolling = false;
        int height = 0;
    };
} // ntgcalls
