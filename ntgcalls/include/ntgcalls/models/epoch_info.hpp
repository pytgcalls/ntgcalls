//
// Created by Laky-64 on 21/06/26.
//

#pragma once

#include <ntgcalls/tl/e2e_api.hpp>
#include <ntgcalls/tl/tl.hpp>

namespace telegram::e2e {
    struct EpochInfo {
        int32_t epoch = 0;
        Hash256 epochHash{};
        int64_t userId = 0;
        bytes::binary secret;
        chain::GroupState groupState;
    };
} // telegram
