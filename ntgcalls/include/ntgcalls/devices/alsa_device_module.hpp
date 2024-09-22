//
// Created by Laky64 on 18/09/24.
//

#pragma once

#ifdef IS_LINUX
#include <alsa/asoundlib.h>
#include <ntgcalls/devices/base_device_module.hpp>
#include <ntgcalls/devices/device_info.hpp>

namespace ntgcalls {

    class AlsaDeviceModule final: public BaseDeviceModule {
        snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
        snd_pcm_t *captureHandle{};
        snd_pcm_hw_params_t *hwParams{};
        std::string deviceID;

        static std::map<std::string, std::string> getDevices(_snd_pcm_stream stream);

    public:
        AlsaDeviceModule(const AudioDescription* desc, bool isCapture);

        [[nodiscard]] bytes::unique_binary read(int64_t size) override;

        static bool isSupported();

        static std::vector<DeviceInfo> getDevices();

        void close() override;
    };

} // alsa

#endif