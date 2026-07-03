//
// Created by Lauren on 18/09/24.
//

#ifdef IS_LINUX
#include <modules/audio_device/linux/audio_device_alsa_linux.h>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/devices/alsa_device_module.hpp>

#define LATE(sym) \
LATESYM_GET(webrtc::adm_linux_alsa::AlsaSymbolTable, GetAlsaSymbolTable(), sym)

#undef snd_ctl_card_info_alloca
#define snd_ctl_card_info_alloca(ptr) \
do { \
*ptr = (snd_ctl_card_info_t*)__builtin_alloca( \
LATE(snd_ctl_card_info_sizeof)()); \
std::memset(*ptr, 0, LATE(snd_ctl_card_info_sizeof)()); \
} while (0)

#undef snd_pcm_info_alloca
#define snd_pcm_info_alloca(pInfo) \
do { \
*pInfo = (snd_pcm_info_t*)__builtin_alloca(LATE(snd_pcm_info_sizeof)()); \
std::memset(*pInfo, 0, LATE(snd_pcm_info_sizeof)()); \
} while (0)

namespace ntgcalls::media::devices {
    AlsaDeviceModule::AlsaDeviceModule(const AudioDescription* desc, const bool is_capture, BaseSink *sink): BaseIO(sink), BaseDeviceModule(desc, is_capture), ThreadedReader(sink), AudioMixer(sink) {
        try {
            device_id_ = device_metadata_["id"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        if (const auto err = LATE(snd_pcm_open)(&alsa_handle_, device_id_.c_str(), is_capture ? SND_PCM_STREAM_CAPTURE:SND_PCM_STREAM_PLAYBACK, 0); err < 0) {
            throw MediaDeviceError("cannot open audio device " + device_id_ + " (" + LATE(snd_strerror)(err) + ")");
        }
        LATE(snd_pcm_hw_params_malloc)(&hw_params_);
        if (const auto err = LATE(snd_pcm_hw_params_any)(alsa_handle_, hw_params_); err < 0) {
            throw MediaDeviceError("cannot initialize hardware parameter structure (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_access)(alsa_handle_, hw_params_, SND_PCM_ACCESS_RW_INTERLEAVED); err < 0) {
            throw MediaDeviceError("cannot set access type (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_format)(alsa_handle_, hw_params_, format_); err < 0) {
            throw MediaDeviceError("cannot set sample format (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_rate_near)(alsa_handle_, hw_params_, &rate_, nullptr); err < 0) {
            throw MediaDeviceError("cannot set sample rate (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params_set_channels)(alsa_handle_, hw_params_, channels_); err < 0) {
            throw MediaDeviceError("cannot set channel count (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        if (const auto err = LATE(snd_pcm_hw_params)(alsa_handle_, hw_params_); err < 0) {
            throw MediaDeviceError("cannot set parameters (" + std::string(LATE(snd_strerror)(err)) + ")");
        }
        LATE(snd_pcm_hw_params_free)(hw_params_);
        if (const auto err = LATE(snd_pcm_prepare)(alsa_handle_); err < 0) {
            throw MediaDeviceError("cannot prepare audio interface for use (" + device_id_ + " " + std::string(LATE(snd_strerror)(err)) + ")");
        }
    }

    AlsaDeviceModule::~AlsaDeviceModule() {
        LATE(snd_pcm_close)(alsa_handle_);
    }

    void AlsaDeviceModule::on_data(const bytes::unique_binary data) {
        LATE(snd_pcm_writei)(alsa_handle_, data.get(), sink_->frame_size() / (channels_ * sizeof(int16_t)));
    }

    bool AlsaDeviceModule::is_supported() {
        return GetAlsaSymbolTable()->Load();
    }

    std::map<std::string, std::string> AlsaDeviceModule::get_devices(const _snd_pcm_stream stream) {
        int card = -1;
        snd_ctl_t *handle;
        snd_ctl_card_info_t *info;
        snd_ctl_card_info_alloca(&info);
        snd_pcm_info_t *pcm_info;
        snd_pcm_info_alloca(&pcm_info);
        std::map<std::string, std::string> devices;
        while (!LATE(snd_card_next)(&card) && card >= 0) {
            const std::string card_name = "hw:" + std::to_string(card);
            if (LATE(snd_ctl_open)(&handle, card_name.c_str(), 0) < 0 ) {
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
                LATE(snd_pcm_info_set_device)(pcm_info, dev);
                LATE(snd_pcm_info_set_subdevice)(pcm_info, 0);
                LATE(snd_pcm_info_set_stream)(pcm_info, stream);
                if (LATE(snd_ctl_pcm_info)(handle, pcm_info) < 0) {
                    continue;
                }
                const char *dev_name = LATE(snd_ctl_card_info_get_name)(info);
                auto id = "hw:" + std::to_string(card) + "," + std::to_string(dev);
                devices[id] = dev_name;
            }
            LATE(snd_ctl_close)(handle);
        }
        return devices;
    }

    std::vector<DeviceInfo> AlsaDeviceModule::get_devices() {
        auto append_device = [](std::vector<DeviceInfo>& devices, const char* name, const char* desc, const bool is_capture) {
            const json metadata = {
                {"is_microphone", is_capture},
                {"id", name},
            };
            devices.emplace_back(desc, metadata.dump());
        };
        std::vector<DeviceInfo> devices;
        auto capture_devices = get_devices(SND_PCM_STREAM_CAPTURE);
        auto playback_devices = get_devices(SND_PCM_STREAM_PLAYBACK);
        for (const auto& [id, name]: capture_devices) {
            append_device(devices, id.c_str(), name.c_str(), true);
        }
        for (const auto& [id, name]: playback_devices) {
            append_device(devices, id.c_str(), name.c_str(), false);
        }
        return devices;
    }

    void AlsaDeviceModule::open() {
        if (is_capture_) {
            run([this](const int64_t size) {
                auto device_data = bytes::make_unique_binary(size);
                if (const auto err = LATE(snd_pcm_readi)(alsa_handle_, device_data.get(), size / (channels_ * sizeof(int16_t))); err < 0) {
                    throw MediaDeviceError("cannot read from audio interface (" + std::string(LATE(snd_strerror)(static_cast<int>(err))) + ")");
                }
                return std::move(device_data);
            });
        }
    }
} // ntgcalls::media::devices

#endif