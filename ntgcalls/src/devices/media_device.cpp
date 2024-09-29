//
// Created by Laky64 on 17/09/24.
//

#include <ntgcalls/devices/media_device.hpp>

#include <utility>
#include <ntgcalls/exceptions.hpp>

#ifdef IS_LINUX
#include <ntgcalls/devices/alsa_device_module.hpp>
#include <ntgcalls/devices/pulse_device_module.hpp>
#elif IS_WINDOWS
#include <ntgcalls/devices/win_core_device_module.hpp>
#endif

namespace ntgcalls {
    std::unique_ptr<BaseReader> MediaDevice::CreateInput(const BaseMediaDescription& desc, BaseSink *sink) {
        if (auto* audio = dynamic_cast<const AudioDescription*>(&desc)) {
            return CreateAudioInput(audio, sink);
        }
        throw MediaDeviceError("Unsupported media type");
    }

    std::vector<DeviceInfo> MediaDevice::GetAudioDevices() {
#ifdef IS_LINUX
        if (PulseDeviceModule::isSupported()) {
            return PulseDeviceModule::getDevices();
        }
        if (AlsaDeviceModule::isSupported()) {
            return AlsaDeviceModule::getDevices();
        }
#elif IS_WINDOWS
        if (WinCoreDeviceModule::isSupported()) {
            return WinCoreDeviceModule::getDevices();
        }
#elif IS_ANDROID
        auto appendDevices = [](std::vector<DeviceInfo>& devices, const std::string& name, const bool& isCapture) {
            json data = {
                {"is_microphone", isCapture},
            };
            devices.emplace_back(name, data.dump());
        };
        std::vector<DeviceInfo> devices;
        appendDevices(devices, "default", true);
        appendDevices(devices, "default", false);
        return devices;
#endif
        return {};
    }

    std::unique_ptr<BaseReader> MediaDevice::CreateAudioInput(const AudioDescription* desc, BaseSink *sink) {
#ifdef IS_LINUX
        if (PulseDeviceModule::isSupported()) {
            RTC_LOG(LS_INFO) << "Using PulseAudio module for input";
            return std::make_unique<PulseDeviceModule>(desc, true, sink);
        }
        if (AlsaDeviceModule::isSupported()) {
            RTC_LOG(LS_INFO) << "Using ALSA module for input";
            return std::make_unique<AlsaDeviceModule>(desc, true, sink);
        }
#elif IS_WINDOWS
        if (WinCoreDeviceModule::isSupported()) {
            RTC_LOG(LS_INFO) << "Using Windows Core Audio module for input";
            adm = std::make_unique<WinCoreDeviceModule>(desc, true);
        }
#endif
        if (adm) {
            return std::make_unique<InputDevice>(std::move(adm), bufferSize);
        }
        throw MediaDeviceError("Unsupported platform for audio device");
    }
} // ntgcalls