//
// Created by Laky64 on 17/09/24.
//

#include <ntgcalls/devices/media_device.hpp>

#include <ntgcalls/devices/audio/audio_device.hpp>
#include <utility>
#include <ntgcalls/exceptions.hpp>

namespace ntgcalls {
    std::unique_ptr<BaseReader> MediaDevice::create(const BaseMediaDescription& desc, std::string deviceId, int64_t bufferSize) {
        if (auto* audio = dynamic_cast<const AudioDescription*>(&desc)) {
            return AudioDevice::create(audio, std::move(deviceId), bufferSize);
        }
        throw MediaDeviceError("Unsupported media type");
    }
} // ntgcalls