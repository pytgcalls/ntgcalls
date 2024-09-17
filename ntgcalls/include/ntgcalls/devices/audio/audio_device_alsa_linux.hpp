//
// Created by Laky64 on 17/09/24.
//

#pragma once

#ifdef IS_LINUX
#include "audio_device.hpp"
#include <alsa/asoundlib.h>

namespace ntgcalls {

    class AudioDeviceAlsaLinux final : public AudioDevice {
        snd_pcm_t *capture_handle{};
        snd_pcm_hw_params_t *hw_params{};
        snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
        uint32_t rate = 0;
        uint32_t framesCount = 0;

    protected:
        bytes::unique_binary readInternal(int64_t size) override;

        void close() override;

    public:
        AudioDeviceAlsaLinux(const AudioDescription* desc, const std::string& deviceId, int64_t bufferSize);
    };

} // wrtc
#endif