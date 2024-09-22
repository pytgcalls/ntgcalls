//
// Created by Laky64 on 18/09/24.
//

#include "ntgcalls/devices/pulse_device_module.hpp"

#ifdef IS_LINUX

#include <ntgcalls/exceptions.hpp>
#include <modules/audio_device/linux/audio_device_pulse_linux.h>

#define LATE(sym) \
LATESYM_GET(webrtc::adm_linux_pulse::PulseAudioSymbolTable, GetPulseSymbolTable(), sym)

namespace ntgcalls {
    PulseDeviceModule::PulseDeviceModule(const AudioDescription* desc, const bool isCapture): BaseDeviceModule(desc, isCapture) {
        pulseConnection = std::make_unique<PulseConnection>();
        RTC_LOG(LS_VERBOSE) << "PulseAudio version: " << pulseConnection->getVersion();

        pa_sample_spec sampleSpec;
        sampleSpec.channels = channels;
        sampleSpec.format = PA_SAMPLE_S16LE;
        sampleSpec.rate = rate;
        std::string deviceID;
        try {
            deviceID = deviceMetadata["id"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        pulseConnection->setupStream(sampleSpec, deviceID, isCapture);
    }

    bytes::unique_binary PulseDeviceModule::read(const int64_t size) {
        return std::move(pulseConnection->read(size));
    }

    bool PulseDeviceModule::isSupported() {
        return GetPulseSymbolTable()->Load();
    }

    std::vector<DeviceInfo> PulseDeviceModule::getDevices() {
        auto appendDevice = [](std::vector<DeviceInfo>& devices, std::string name, const std::string& desc, const bool isCapture) {
            const json metadata = {
                {"is_microphone", isCapture},
                {"id", name},
            };
            devices.emplace_back(desc, metadata.dump());
        };
        const auto pulseConnection = std::make_unique<PulseConnection>();
        auto playDevices = pulseConnection->getPlayDevices();
        auto recordDevices = pulseConnection->getRecordDevices();
        pulseConnection->disconnect();
        std::vector<DeviceInfo> devices;
        for (const auto& [fst, snd]: playDevices) {
            appendDevice(devices, fst, snd, false);
        }
        for (const auto& [fst, snd]: recordDevices) {
            appendDevice(devices, fst, snd, true);
        }
        return devices;
    }

    void PulseDeviceModule::close() {
        pulseConnection->disconnect();
    }
} // pulse

#endif