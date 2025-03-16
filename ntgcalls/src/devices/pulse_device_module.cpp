//
// Created by Laky64 on 18/09/24.
//

#include <ntgcalls/devices/pulse_device_module.hpp>

#ifdef IS_LINUX

#include <ntgcalls/exceptions.hpp>
#include <modules/audio_device/linux/audio_device_pulse_linux.h>

#define LATE(sym) \
LATESYM_GET(webrtc::adm_linux_pulse::PulseAudioSymbolTable, GetPulseSymbolTable(), sym)

namespace ntgcalls {
    PulseDeviceModule::PulseDeviceModule(const AudioDescription* desc, const bool isCapture, BaseSink *sink): BaseIO(sink), BaseDeviceModule(desc, isCapture), BaseReader(sink), AudioMixer(sink) {
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

    PulseDeviceModule::~PulseDeviceModule() {
        running = false;
        pulseConnection->disconnect();
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
        auto recordDevices = pulseConnection->getRecordDevices();
        auto playDevices = pulseConnection->getPlayDevices();
        pulseConnection->disconnect();
        std::vector<DeviceInfo> devices;
        for (const auto& [fst, snd]: recordDevices) {
            appendDevice(devices, fst, snd, true);
        }
        for (const auto& [fst, snd]: playDevices) {
            appendDevice(devices, fst, snd, false);
        }
        return devices;
    }

    void PulseDeviceModule::open() {
        if (running) return;
        running = true;
        pulseConnection->start(sink->frameSize());
        if (isCapture) {
            pulseConnection->onData([this](bytes::unique_binary data) {
                dataCallback(std::move(data), {});
            });
        }
    }

    void PulseDeviceModule::onData(const bytes::unique_binary data) {
        if (!running) return;
        pulseConnection->writeData(data, sink->frameSize());
    }
} // pulse

#endif