//
// Created by Lauren on 18/09/24.
//

#pragma once

#ifdef IS_LINUX
#include <ntgcalls/io/audio_mixer.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/devices/base_device_module.hpp>
#include <ntgcalls/media/devices/device_info.hpp>
#include <ntgcalls/utils/pulse_connection.hpp>

namespace ntgcalls::media::devices {

    class PulseDeviceModule final: public BaseDeviceModule, public io::BaseReader, public io::AudioMixer {
        std::unique_ptr<utils::PulseConnection> pulse_connection_;

    protected:
        void on_data(bytes::unique_binary data) override;

    public:
        PulseDeviceModule(const AudioDescription* desc, bool is_capture, BaseSink *sink);

        ~PulseDeviceModule() override;

        static bool is_supported();

        static std::vector<DeviceInfo> get_devices();

        void open() override;
    };

} // ntgcalls::media::devices

#endif