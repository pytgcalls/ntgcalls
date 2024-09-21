//
// Created by Laky64 on 18/09/24.
//

#include "ntgcalls/devices/base_device_module.hpp"

#include <ntgcalls/exceptions.hpp>

namespace ntgcalls {
    BaseDeviceModule::BaseDeviceModule(const AudioDescription* desc): rate(desc->sampleRate), channels(desc->channelCount) {
        try {
            deviceMetadata = json::parse(desc->input);
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
    }
} // ntgcalls