//
// Created by Laky64 on 21/09/2024.
//

#pragma once
#include <string>

namespace ntgcalls {

    class DeviceInfo {
    public:
        std::string name;
        std::string metadata;

        DeviceInfo(std::string name, std::string metadata) : name(std::move(name)), metadata(std::move(metadata)) {}
    };

} // ntgcalls
