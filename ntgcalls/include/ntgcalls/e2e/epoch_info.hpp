//
// Created by Lauren on 21/06/26.
//

#pragma once

#include <ntgcalls/tl/e2e_api.hpp>
#include <ntgcalls/tl/tl.hpp>

namespace ntgcalls::e2e {
    struct EpochInfo {
        int32_t epoch = 0;
        tl::Hash256 epoch_hash{};
        int64_t user_id = 0;
        bytes::binary secret;
        chain::GroupState group_state;
    };
} // ntgcalls::e2e
