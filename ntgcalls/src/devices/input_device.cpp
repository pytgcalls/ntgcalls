//
// Created by Laky64 on 18/09/24.
//

#include <ntgcalls/devices/input_device.hpp>

#ifdef IS_LINUX

namespace ntgcalls {
    InputDevice::InputDevice(const AudioDescription* desc, std::unique_ptr<BaseDeviceModule> adm, const int64_t bufferSize): BaseReader(bufferSize, true), adm(adm) {}

    bytes::unique_binary InputDevice::readInternal(const int64_t size) {
        return std::move(adm->read(size));
    }

    void InputDevice::close() {
        adm = nullptr;
    }
} // alsa

#endif