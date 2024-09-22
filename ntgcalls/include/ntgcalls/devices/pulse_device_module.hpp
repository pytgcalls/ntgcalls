//
// Created by Laky64 on 18/09/24.
//

#pragma once

#ifdef IS_LINUX
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/devices/base_device_module.hpp>
#include <ntgcalls/utils/pulse_connection.hpp>

namespace ntgcalls {

    class PulseDeviceModule final: public BaseDeviceModule {
        std::unique_ptr<PulseConnection> pulseConnection;

    public:
        PulseDeviceModule(const AudioDescription* desc, bool isCapture);

        [[nodiscard]] bytes::unique_binary read(int64_t size) override;

        static bool isSupported();

        static std::vector<DeviceInfo> getDevices();

        void close() override;
    };

} // pulse

#endif