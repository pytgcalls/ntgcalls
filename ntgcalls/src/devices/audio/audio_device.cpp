//
// Created by Laky64 on 17/09/24.
//

#include <utility>

#include "ntgcalls/devices/audio/audio_device.hpp"

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/devices/audio/audio_device_alsa_linux.hpp>

namespace ntgcalls {
    AudioDevice::AudioDevice(std::string deviceId, const int64_t bufferSize): BaseReader(bufferSize, true), deviceId(std::move(deviceId)) {}

    std::unique_ptr<AudioDevice> AudioDevice::create(const AudioDescription* desc, const std::string& deviceId, int64_t bufferSize) {
#ifdef IS_LINUX
        return std::make_unique<AudioDeviceAlsaLinux>(desc, deviceId, bufferSize);
#else
        throw MediaDeviceError("Unsupported platform for audio device");
#endif
    }
} // wrtc
