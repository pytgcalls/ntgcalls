//
// Created by Laky64 on 17/09/24.
//

#include <ntgcalls/devices/audio/audio_device_alsa_linux.hpp>

#ifdef IS_LINUX
#include <ntgcalls/exceptions.hpp>

namespace ntgcalls {
    AudioDeviceAlsaLinux::AudioDeviceAlsaLinux(const AudioDescription* desc, const std::string& deviceId, const int64_t bufferSize): AudioDevice(deviceId, bufferSize) {
        rate = desc->sampleRate;
        framesCount = desc->sampleRate * snd_pcm_format_width(format) / 8 / 100 / desc->channelCount;
        if (const auto err = snd_pcm_open(&capture_handle, deviceId.c_str(), SND_PCM_STREAM_CAPTURE, 0); err < 0) {
            throw MediaDeviceError("cannot open audio device " + deviceId + " (" + snd_strerror(err) + ")");
        }
        snd_pcm_hw_params_malloc(&hw_params);
        if (const auto err = snd_pcm_hw_params_any(capture_handle, hw_params); err < 0) {
            throw MediaDeviceError("cannot initialize hardware parameter structure (" + std::string(snd_strerror(err)) + ")");
        }
        if (const auto err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED); err < 0) {
            throw MediaDeviceError("cannot set access type (" + std::string(snd_strerror(err)) + ")");
        }
        if (const auto err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format); err < 0) {
            throw MediaDeviceError("cannot set sample format (" + std::string(snd_strerror(err)) + ")");
        }
        if (const auto err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, nullptr); err < 0) {
            throw MediaDeviceError("cannot set sample rate (" + std::string(snd_strerror(err)) + ")");
        }
        if (const auto err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, desc->channelCount); err < 0) {
            throw MediaDeviceError("cannot set channel count (" + std::string(snd_strerror(err)) + ")");
        }
        if (const auto err = snd_pcm_hw_params(capture_handle, hw_params); err < 0) {
            throw MediaDeviceError("cannot set parameters (" + std::string(snd_strerror(err)) + ")");
        }
        snd_pcm_hw_params_free(hw_params);
        if (const auto err = snd_pcm_prepare(capture_handle); err < 0) {
            throw MediaDeviceError("cannot prepare audio interface for use (" + deviceId + " " + std::string(snd_strerror(err)) + ")");
        }
    }

    bytes::unique_binary AudioDeviceAlsaLinux::readInternal(const int64_t size) {
        auto device_data = bytes::make_unique_binary(size);
        if (const auto err = snd_pcm_readi(capture_handle, device_data.get(), framesCount); err < 0) {
            throw MediaDeviceError("cannot read from audio interface (" + std::string(snd_strerror(static_cast<int>(err))) + ")");
        }
        return std::move(device_data);
    }

    void AudioDeviceAlsaLinux::close() {
        snd_pcm_close(capture_handle);
    }
} // ntgcalls
#endif