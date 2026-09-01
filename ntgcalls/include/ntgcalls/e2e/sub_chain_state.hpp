//
// Created by Lauren on 19/06/26.
//

#pragma once
#include <cstdint>
#include <map>

namespace ntgcalls::e2e {
    struct SubChainState {
        int short_poll_generation = 0;
        int waiting_generation = 0;
        bool waiting_active = false;
        int64_t last_update = 0;
        std::map<int, bytes::binary> waiting;
        bool short_polling = false;
        int height = 0;
    };
} // ntgcalls::e2e
