//
// Created by Lauren on 07/03/24.
//

#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ntgcalls::p2p {
    struct Protocol {
        int32_t min_layer;
        int32_t max_layer;
        bool udp_p2p;
        bool udp_reflector;
        std::vector<std::string> library_versions;
    };
} // ntgcalls::p2p
