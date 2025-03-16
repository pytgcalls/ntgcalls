//
// Created by Laky64 on 21/09/2024.
//

#pragma once
#include <ntgcalls/devices/device_info.hpp>

namespace ntgcalls {

    struct MediaDevices {
        std::vector<DeviceInfo> microphone{};
        std::vector<DeviceInfo> speaker{};
        std::vector<DeviceInfo> camera{};
        std::vector<DeviceInfo> screen{};
    };

} // ntgcalls
