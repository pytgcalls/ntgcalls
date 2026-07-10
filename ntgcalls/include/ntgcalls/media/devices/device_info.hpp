//
// Created by Lauren on 21/09/24.
//

#pragma once
#include <string>

namespace ntgcalls::media::devices {

    class DeviceInfo {
    public:
        std::string name;
        std::string metadata;

        DeviceInfo(std::string name, std::string metadata) : name(std::move(name)), metadata(std::move(metadata)) {}
    };

} // ntgcalls
