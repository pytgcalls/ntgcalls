//
// Created by Laky64 on 18/09/24.
//

#pragma once

#ifdef IS_LINUX
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/devices/base_device_module.hpp>
#include <ntgcalls/utils/pulse_connection.hpp>

namespace ntgcalls {

    class PulseDeviceModule final: public BaseDeviceModule, public BaseReader {
        std::unique_ptr<PulseConnection> pulseConnection;

    public:
        PulseDeviceModule(const AudioDescription* desc, bool isCapture, BaseSink *sink);

        ~PulseDeviceModule() override;

        static bool isSupported();

        static std::vector<DeviceInfo> getDevices();

        void open() override;
    };

} // pulse

#endif