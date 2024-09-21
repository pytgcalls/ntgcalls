//
// Created by Laky64 on 18/09/24.
//

#include "ntgcalls/devices/base_device_module.hpp"
#include <sstream>

namespace ntgcalls {
    [[nodiscard]] std::vector<std::string> BaseDeviceModule::extractMetadata() const {
        std::stringstream ss(deviceId);
        std::string item;
        std::vector<std::string> tokens;
        while (std::getline(ss, item, ';')) {
            tokens.push_back(item);
        }
        return tokens;
    }
} // ntgcalls