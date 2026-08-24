//
// Created by Lauren on 18/09/24.
//

#pragma once

#ifdef IS_LINUX
#include <alsa/asoundlib.h>
#include <ntgcalls/io/audio_mixer.hpp>
#include <ntgcalls/io/threaded_reader.hpp>
#include <ntgcalls/media/devices/base_device_module.hpp>
#include <ntgcalls/media/devices/device_info.hpp>

namespace ntgcalls::media::devices {

    class AlsaDeviceModule final: public BaseDeviceModule, public io::ThreadedReader, public io::AudioMixer {
        snd_pcm_format_t format_ = SND_PCM_FORMAT_S16_LE;
        snd_pcm_t* alsa_handle_{};
        snd_pcm_hw_params_t* hw_params_{};
        std::string device_id_;

        static std::map<std::string, std::string> get_devices(_snd_pcm_stream stream);

    protected:
        void on_data(bytes::unique_binary data) override;

    public:
        AlsaDeviceModule(const AudioDescription* desc, bool is_capture, BaseSink* sink);

        ~AlsaDeviceModule() override;

        static bool is_supported();

        static std::vector<DeviceInfo> get_devices();

        void open() override;
    };

} // ntgcalls::media::devices

#endif
