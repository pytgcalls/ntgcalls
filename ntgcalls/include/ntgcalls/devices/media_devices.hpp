//
// Created by Laky64 on 21/09/2024.
//

#pragma once
#include "device_info.hpp"

namespace ntgcalls {

    struct MediaDevices {
        std::vector<DeviceInfo> audio{};
        std::vector<DeviceInfo> video{};
    };

} // ntgcalls
