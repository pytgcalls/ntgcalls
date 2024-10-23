//
// Created by Laky64 on 18/09/24.
//

#include <ntgcalls/devices/base_device_module.hpp>

#include <ntgcalls/exceptions.hpp>

namespace ntgcalls {
    BaseDeviceModule::BaseDeviceModule(const AudioDescription* desc, const bool isCapture): rate(desc->sampleRate), channels(desc->channelCount), isCapture(isCapture) {
        auto isMicrophone = false;
        try {
            deviceMetadata = json::parse(desc->input);
            isMicrophone = deviceMetadata["is_microphone"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        if (isMicrophone != isCapture) {
            throw MediaDeviceError("Using microphone as speaker or vice versa");
        }
    }
} // ntgcalls