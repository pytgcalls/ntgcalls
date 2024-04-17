//
// Created by Laky64 on 07/03/2024.
//

#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ntgcalls {
    struct Protocol {
        int32_t min_layer;
        int32_t max_layer;
        bool udp_p2p;
        bool udp_reflector;
        std::vector<std::string> library_versions;
    };
}
