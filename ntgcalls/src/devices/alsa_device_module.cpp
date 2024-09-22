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
    AlsaDeviceModule::AlsaDeviceModule(const AudioDescription* desc, const bool isCapture): BaseDeviceModule(desc, isCapture) {
        try {
            deviceID = deviceMetadata["id"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        if (const auto err = LATE(snd_pcm_open)(&captureHandle, deviceID.c_str(), isCapture ? SND_PCM_STREAM_CAPTURE:SND_PCM_STREAM_PLAYBACK, 0); err < 0) {
            throw MediaDeviceError("cannot open audio device " + deviceID + " (" + LATE(snd_strerror)(err) + ")");
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
            throw MediaDeviceError("cannot prepare audio interface for use (" + deviceID + " " + std::string(LATE(snd_strerror)(err)) + ")");
        }
    }

    bytes::unique_binary AlsaDeviceModule::read(const int64_t size) {
        auto device_data = bytes::make_unique_binary(size);
        if (const auto err = LATE(snd_pcm_readi)(captureHandle, device_data.get(), size / (channels * 2)); err < 0) {
            throw MediaDeviceError("cannot read from audio interface (" + std::string(LATE(snd_strerror)(static_cast<int>(err))) + ")");
        }
        return std::move(device_data);
    }

    bool AlsaDeviceModule::isSupported() {
        return GetAlsaSymbolTable()->Load();
    }

    std::vector<DeviceInfo> AlsaDeviceModule::getDevices() {
        int card = -1;
        auto appendDevice = [](std::vector<DeviceInfo>& devices, const char* name, const char* desc, const bool isCapture) {
            const json metadata = {
                {"is_microphone", isCapture},
                {"id", name},
            };
            devices.emplace_back(desc, metadata.dump());
        };
        std::vector<DeviceInfo> devices;
        while (!LATE(snd_card_next)(&card) && card >= 0) {
            char **hints;
            if (const auto err = LATE(snd_device_name_hint)(card, "pcm", reinterpret_cast<void***>(&hints)) < 0) {
                throw MediaDeviceError("Error getting device hints (" + std::string(LATE(snd_strerror)(err)) + ")");
            }
            char **n = hints;
            while (*n != nullptr) {
                char *name = LATE(snd_device_name_get_hint)(*n, "NAME");
                char *desc = LATE(snd_device_name_get_hint)(*n, "DESC");
                char *io = LATE(snd_device_name_get_hint)(*n, "IOID");
                if (name && desc) {
                    if (io) {
                        appendDevice(devices, name, desc, strcmp(io, "Input") == 0);
                    } else {
                        appendDevice(devices, name, desc, true);
                        appendDevice(devices, name, desc, false);
                    }
                }
                if (name)
                    free(name);
                if (desc)
                    free(desc);
                if (io)
                    free(io);
                n++;
            }
            LATE(snd_device_name_free_hint)(reinterpret_cast<void**>(hints));
        }
        return devices;
    }

    void AlsaDeviceModule::close() {
        LATE(snd_pcm_close)(captureHandle);
    }
} // alsa

#endif