//
// Created by Laky64 on 18/09/24.
//

#pragma once
#include <ntgcalls/io/audio_mixer.hpp>

#ifdef IS_LINUX
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/devices/base_device_module.hpp>
#include <ntgcalls/utils/pulse_connection.hpp>

namespace ntgcalls {

    class PulseDeviceModule final: public BaseDeviceModule, public BaseReader, public AudioMixer {
        std::unique_ptr<PulseConnection> pulseConnection;

        void onData(bytes::unique_binary data) override;

    public:
        PulseDeviceModule(const AudioDescription* desc, bool isCapture, BaseSink *sink);

        ~PulseDeviceModule() override;

        static bool isSupported();

        static std::vector<DeviceInfo> getDevices();

        void open() override;
    };

} // pulse

#endif