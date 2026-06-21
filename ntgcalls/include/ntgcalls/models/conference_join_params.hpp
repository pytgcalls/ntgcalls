//
// Created by Laky-64 on 15/06/26.
//

#pragma once
#include <string>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls {
    struct ConferenceJoinParams {
        std::string payload;
        bytes::binary publicKey;
        bytes::binary block;
    };
} // ntgcalls
