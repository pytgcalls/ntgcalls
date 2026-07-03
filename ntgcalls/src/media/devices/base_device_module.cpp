//
// Created by Lauren on 18/09/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/devices/base_device_module.hpp>

namespace ntgcalls::media::devices {
    BaseDeviceModule::BaseDeviceModule(const AudioDescription* desc, const bool is_capture): rate_(desc->sample_rate), channels_(desc->channel_count), is_capture_(is_capture) {
        auto is_microphone = false;
        try {
            device_metadata_ = json::parse(desc->input);
            is_microphone = device_metadata_["is_microphone"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        if (is_microphone != is_capture) {
            throw MediaDeviceError("Using microphone as speaker or vice versa");
        }
    }
} // ntgcalls::media::devices