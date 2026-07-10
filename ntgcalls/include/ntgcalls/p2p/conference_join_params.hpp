//
// Created by Lauren on 15/06/26.
//

#pragma once
#include <string>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::p2p {
    struct ConferenceJoinParams {
        std::string payload;
        bytes::binary public_key;
        bytes::binary block;
    };
} // ntgcalls
