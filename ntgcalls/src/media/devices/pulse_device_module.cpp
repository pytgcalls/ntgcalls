//
// Created by Lauren on 18/09/24.
//

#include <ntgcalls/media/devices/pulse_device_module.hpp>

#ifdef IS_LINUX

#include <modules/audio_device/linux/audio_device_pulse_linux.h>
#include <ntgcalls/exceptions.hpp>

#define LATE(sym) \
LATESYM_GET(webrtc::adm_linux_pulse::PulseAudioSymbolTable, GetPulseSymbolTable(), sym)

namespace ntgcalls::media::devices {
    PulseDeviceModule::PulseDeviceModule(const AudioDescription* desc, const bool is_capture, BaseSink *sink): BaseIO(sink), BaseDeviceModule(desc, is_capture), BaseReader(sink), AudioMixer(sink) {
        pulse_connection_ = std::make_unique<utils::PulseConnection>();
        RTC_LOG(LS_VERBOSE) << "PulseAudio version: " << pulse_connection_->get_version();

        pa_sample_spec sample_spec;
        sample_spec.channels = channels_;
        sample_spec.format = PA_SAMPLE_S16LE;
        sample_spec.rate = rate_;
        std::string device_id;
        try {
            device_id = device_metadata_["id"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        pulse_connection_->setup_stream(sample_spec, device_id, is_capture);
    }

    PulseDeviceModule::~PulseDeviceModule() {
        running_ = false;
        pulse_connection_->disconnect();
    }

    bool PulseDeviceModule::is_supported() {
        return GetPulseSymbolTable()->Load();
    }

    std::vector<DeviceInfo> PulseDeviceModule::get_devices() {
        auto append_device = [](std::vector<DeviceInfo>& devices, std::string name, const std::string& desc, const bool is_capture) {
            const json metadata = {
                {"is_microphone", is_capture},
                {"id", name},
            };
            devices.emplace_back(desc, metadata.dump());
        };
        const auto pulse_connection = std::make_unique<utils::PulseConnection>();
        auto record_devices = pulse_connection->get_record_devices();
        auto play_devices = pulse_connection->get_play_devices();
        pulse_connection->disconnect();
        std::vector<DeviceInfo> devices;
        for (const auto& [fst, snd]: record_devices) {
            append_device(devices, fst, snd, true);
        }
        for (const auto& [fst, snd]: play_devices) {
            append_device(devices, fst, snd, false);
        }
        return devices;
    }

    void PulseDeviceModule::open() {
        if (running_) return;
        running_ = true;
        pulse_connection_->start(sink_->frame_size());
        if (is_capture_) {
            pulse_connection_->on_data([this](bytes::unique_binary data) {
                data_callback_(std::move(data), {});
            });
        }
    }

    void PulseDeviceModule::on_data(const bytes::unique_binary data) {
        if (!running_) return;
        pulse_connection_->write_data(data, sink_->frame_size());
    }
} // ntgcalls::media::devices

#endif