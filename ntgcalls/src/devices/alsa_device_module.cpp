//
// Created by Laky64 on 18/09/24.
//

#ifdef IS_LINUX
#include <ntgcalls/devices/alsa_device_module.hpp>
#include <ntgcalls/exceptions.hpp>
#include <modules/audio_device/linux/audio_device_alsa_linux.h>

#define LATE(sym) \
LATESYM_GET(webrtc::adm_linux_alsa::AlsaSymbolTable, GetAlsaSymbolTable(), sym)

#undef snd_ctl_card_info_alloca
#define snd_ctl_card_info_alloca(ptr) \
do { \
*ptr = (snd_ctl_card_info_t*)__builtin_alloca( \
LATE(snd_ctl_card_info_sizeof)()); \
memset(*ptr, 0, LATE(snd_ctl_card_info_sizeof)()); \
} while (0)

#undef snd_pcm_info_alloca
#define snd_pcm_info_alloca(pInfo) \
do { \
*pInfo = (snd_pcm_info_t*)__builtin_alloca(LATE(snd_pcm_info_sizeof)()); \
memset(*pInfo, 0, LATE(snd_pcm_info_sizeof)()); \
} while (0)

namespace ntgcalls {
    AlsaDeviceModule::AlsaDeviceModule(const AudioDescription* desc, const bool isCapture, BaseSink *sink): BaseIO(sink), BaseDeviceModule(desc, isCapture), ThreadedReader(sink), AudioMixer(sink) {
        try {
            deviceID = deviceMetadata["id"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        if (const auto err = LATE(snd_pcm_open)(&alsaHandle, deviceID.c_str(), isCapture ? SND_PCM_STREAM_CAPTURE:SND_PCM_STREAM_PLAYBACK, 0); err < 0) {
            throw MediaDeviceError("cannot open audio device " + deviceID + " (" + LATE(snd_strerror)(err) + ")");
        }
        LATE(snd_pcm_hw_params_malloc)(&hwParams);
        if (const auto err = LATE(snd_pcm_hw_params_any)(alsaHandle, hwParams); err < 0) {
            throw MediaDeviceError("cannot initialize hardware parameter structure (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_access)(alsaHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED); err < 0) {
            throw MediaDeviceError("cannot set access type (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_format)(alsaHandle, hwParams, format); err < 0) {
            throw MediaDeviceError("cannot set sample format (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_rate_near)(alsaHandle, hwParams, &rate, nullptr); err < 0) {
            throw MediaDeviceError("cannot set sample rate (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_channels)(alsaHandle, hwParams, channels); err < 0) {
            throw MediaDeviceError("cannot set channel count (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params)(alsaHandle, hwParams); err < 0) {
            throw MediaDeviceError("cannot set parameters (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        LATE(snd_pcm_hw_params_free)(hwParams);
        if (const auto err = LATE(snd_pcm_prepare)(alsaHandle); err < 0) {
            throw MediaDeviceError("cannot prepare audio interface for use (" + deviceID + " " + std::string(LATE(snd_strerror)(err)) + ")");
        }
    }

    AlsaDeviceModule::~AlsaDeviceModule() {
        LATE(snd_pcm_close)(alsaHandle);
    }

    void AlsaDeviceModule::onData(const bytes::unique_binary data) {
        LATE(snd_pcm_writei)(alsaHandle, data.get(), sink->frameSize() / (channels * sizeof(int16_t)));
    }

    bool AlsaDeviceModule::isSupported() {
        return GetAlsaSymbolTable()->Load();
    }

    std::map<std::string, std::string> AlsaDeviceModule::getDevices(const _snd_pcm_stream stream) {
        int card = -1;
        snd_ctl_t *handle;
        snd_ctl_card_info_t *info;
        snd_ctl_card_info_alloca(&info);
        snd_pcm_info_t *pcmInfo;
        snd_pcm_info_alloca(&pcmInfo);
        std::map<std::string, std::string> devices;
        while (!LATE(snd_card_next)(&card) && card >= 0) {
            std::string cardName = "hw:" + std::to_string(card);
            if (LATE(snd_ctl_open)(&handle, cardName.c_str(), 0) < 0 ) {
                continue;
            }
            if (LATE(snd_ctl_card_info)(handle, info) < 0) {
                LATE(snd_ctl_close)(handle);
                continue;
            }
            auto dev = -1;
            while (true) {
                if (LATE(snd_ctl_pcm_next_device)(handle, &dev) < 0) {
                    break;
                }
                if (dev < 0) {
                    break;
                }
                LATE(snd_pcm_info_set_device)(pcmInfo, dev);
                LATE(snd_pcm_info_set_subdevice)(pcmInfo, 0);
                LATE(snd_pcm_info_set_stream)(pcmInfo, stream);
                if (LATE(snd_ctl_pcm_info)(handle, pcmInfo) < 0) {
                    continue;
                }
                const char *devName = LATE(snd_ctl_card_info_get_name)(info);
                auto id = "hw:" + std::to_string(card) + "," + std::to_string(dev);
                devices[id] = devName;
            }
            LATE(snd_ctl_close)(handle);
        }
        return devices;
    }

    std::vector<DeviceInfo> AlsaDeviceModule::getDevices() {
        auto appendDevice = [](std::vector<DeviceInfo>& devices, const char* name, const char* desc, const bool isCapture) {
            const json metadata = {
                {"is_microphone", isCapture},
                {"id", name},
            };
            devices.emplace_back(desc, metadata.dump());
        };
        std::vector<DeviceInfo> devices;
        auto captureDevices = getDevices(SND_PCM_STREAM_CAPTURE);
        auto playbackDevices = getDevices(SND_PCM_STREAM_PLAYBACK);
        for (const auto& [id, name]: captureDevices) {
            appendDevice(devices, id.c_str(), name.c_str(), true);
        }
        for (const auto& [id, name]: playbackDevices) {
            appendDevice(devices, id.c_str(), name.c_str(), false);
        }
        return devices;
    }

    void AlsaDeviceModule::open() {
        if (isCapture) {
            run([this](const int64_t size) {
                auto device_data = bytes::make_unique_binary(size);
                if (const auto err = LATE(snd_pcm_readi)(alsaHandle, device_data.get(), size / (channels * sizeof(int16_t))); err < 0) {
                    throw MediaDeviceError("cannot read from audio interface (" + std::string(LATE(snd_strerror)(static_cast<int>(err))) + ")");
                }
                return std::move(device_data);
            });
        }
    }
} // alsa

#endif