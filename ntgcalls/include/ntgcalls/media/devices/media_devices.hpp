//
// Created by Lauren on 21/09/24.
//

#pragma once
#include <ntgcalls/media/devices/device_info.hpp>

namespace ntgcalls::media::devices {

    struct MediaDevices {
        std::vector<DeviceInfo> microphone{};
        std::vector<DeviceInfo> speaker{};
        std::vector<DeviceInfo> camera{};
        std::vector<DeviceInfo> screen{};
    };

} // ntgcalls::media::devices
