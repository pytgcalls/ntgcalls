//
// Created by Laky64 on 18/09/24.
//

#include <ntgcalls/devices/alsa_device_module.hpp>

#ifdef IS_LINUX

#include <ntgcalls/exceptions.hpp>
#include <modules/audio_device/linux/audio_device_alsa_linux.h>

#define LATE(sym) \
LATESYM_GET(webrtc::adm_linux_alsa::AlsaSymbolTable, GetAlsaSymbolTable(), sym)

namespace ntgcalls {
    AlsaDeviceModule::AlsaDeviceModule(const AudioDescription* desc, const bool isCapture): BaseDeviceModule(desc) {
        if (const auto err = LATE(snd_pcm_open)(&captureHandle, desc->input.c_str(), isCapture ? SND_PCM_STREAM_CAPTURE:SND_PCM_STREAM_PLAYBACK, 0); err < 0) {
            throw MediaDeviceError("cannot open audio device " + desc->input + " (" + LATE(snd_strerror)(err) + ")");
        }
        LATE(snd_pcm_hw_params_malloc)(&hwParams);
        if (const auto err = LATE(snd_pcm_hw_params_any)(captureHandle, hwParams); err < 0) {
            throw MediaDeviceError("cannot initialize hardware parameter structure (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_access)(captureHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED); err < 0) {
            throw MediaDeviceError("cannot set access type (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_format)(captureHandle, hwParams, format); err < 0) {
            throw MediaDeviceError("cannot set sample format (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_rate_near)(captureHandle, hwParams, &rate, nullptr); err < 0) {
            throw MediaDeviceError("cannot set sample rate (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_channels)(captureHandle, hwParams, channels); err < 0) {
            throw MediaDeviceError("cannot set channel count (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params)(captureHandle, hwParams); err < 0) {
            throw MediaDeviceError("cannot set parameters (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        LATE(snd_pcm_hw_params_free)(hwParams);
        if (const auto err = LATE(snd_pcm_prepare)(captureHandle); err < 0) {
            throw MediaDeviceError("cannot prepare audio interface for use (" + desc->input + " " + std::string(LATE(snd_strerror)(err)) + ")");
        }
    }

    AlsaDeviceModule::~AlsaDeviceModule() {
        LATE(snd_pcm_close)(captureHandle);
    }

    bytes::unique_binary AlsaDeviceModule::read(const int64_t size) const {
        auto device_data = bytes::make_unique_binary(size);
        if (const auto err = LATE(snd_pcm_readi)(captureHandle, device_data.get(), size / (channels * 2)); err < 0) {
            throw MediaDeviceError("cannot read from audio interface (" + std::string(LATE(snd_strerror)(static_cast<int>(err))) + ")");
        }
        return std::move(device_data);
    }

    bool AlsaDeviceModule::isSupported() {
        return GetAlsaSymbolTable()->Load();
    }
} // alsa

#endif