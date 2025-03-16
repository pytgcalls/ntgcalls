//
// Created by Laky64 on 18/09/24.
//

#pragma once

#ifdef IS_LINUX
#include <alsa/asoundlib.h>
#include <ntgcalls/io/threaded_reader.hpp>
#include <ntgcalls/devices/base_device_module.hpp>
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/io/audio_mixer.hpp>

namespace ntgcalls {

    class AlsaDeviceModule final: public BaseDeviceModule, public ThreadedReader, public AudioMixer {
        snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
        snd_pcm_t *alsaHandle{};
        snd_pcm_hw_params_t *hwParams{};
        std::string deviceID;

        static std::map<std::string, std::string> getDevices(_snd_pcm_stream stream);

        void onData(bytes::unique_binary data) override;

    public:
        AlsaDeviceModule(const AudioDescription* desc, bool isCapture, BaseSink *sink);

        ~AlsaDeviceModule() override;

        static bool isSupported();

        static std::vector<DeviceInfo> getDevices();

        void open() override;
    };

} // alsa

#endif