//
// Created by Laky64 on 17/09/24.
//

#include <ntgcalls/devices/media_device.hpp>
#include <ntgcalls/devices/desktop_capturer_module.hpp>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/devices/camera_capturer_module.hpp>

#ifdef IS_LINUX
#include <ntgcalls/devices/alsa_device_module.hpp>
#include <ntgcalls/devices/pulse_device_module.hpp>
#elif IS_WINDOWS
#include <ntgcalls/devices/win_core_device_module.hpp>
#elif IS_ANDROID
#include <wrtc/utils/java_context.hpp>
#include <ntgcalls/devices/open_sles_device_module.hpp>
#include <ntgcalls/devices/java_audio_device_module.hpp>
#endif

namespace ntgcalls {
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
        if (OpenSLESDeviceModule::isSupported(static_cast<JNIEnv*>(wrtc::GetJNIEnv()), true) || JavaAudioDeviceModule::isSupported()) {
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
        }
#endif
        return {};
    }

    std::vector<DeviceInfo> MediaDevice::GetScreenDevices() {
#ifndef IS_ANDROID
        if (DesktopCapturerModule::IsSupported()) {
            return DesktopCapturerModule::GetSources();
        }
#endif
        return {};
    }

    std::vector<DeviceInfo> MediaDevice::GetCameraDevices() {
#ifndef IS_ANDROID
        return CameraCapturerModule::GetSources();
#else
        return {};
#endif
    }

    std::unique_ptr<BaseReader> MediaDevice::CreateDesktopCapture(const VideoDescription& desc, BaseSink* sink) {
#ifndef IS_ANDROID
        if (DesktopCapturerModule::IsSupported()) {
            RTC_LOG(LS_INFO) << "Using DesktopCapturer module for input";
            return std::make_unique<DesktopCapturerModule>(desc, sink);
        }
#endif
        throw MediaDeviceError("Unsupported platform for desktop capture");
    }

    std::unique_ptr<BaseReader> MediaDevice::CreateCameraCapture(const VideoDescription& desc, BaseSink* sink) {
#ifndef IS_ANDROID
        return std::make_unique<CameraCapturerModule>(desc, sink);
#else
        throw MediaDeviceError("Unsupported platform for camera capture");
#endif
    }

    std::unique_ptr<BaseIO> MediaDevice::CreateAudioDevice(const AudioDescription* desc, BaseSink *sink, const bool isCapture) {
#ifdef IS_LINUX
        if (PulseDeviceModule::isSupported()) {
            RTC_LOG(LS_INFO) << "Using PulseAudio module for input";
            return std::make_unique<PulseDeviceModule>(desc, isCapture, sink);
        }
        if (AlsaDeviceModule::isSupported()) {
            RTC_LOG(LS_INFO) << "Using ALSA module for input";
            return std::make_unique<AlsaDeviceModule>(desc, isCapture, sink);
        }
#elif IS_WINDOWS
        if (WinCoreDeviceModule::isSupported()) {
            RTC_LOG(LS_INFO) << "Using Windows Core Audio module for input";
            return std::make_unique<WinCoreDeviceModule>(desc, isCapture, sink);
        }
#elif IS_ANDROID
        if (OpenSLESDeviceModule::isSupported(static_cast<JNIEnv*>(wrtc::GetJNIEnv()), true)) {
            RTC_LOG(LS_INFO) << "Using OpenSLES module for input";
            return std::make_unique<OpenSLESDeviceModule>(desc, isCapture, sink);
        }
        if (JavaAudioDeviceModule::isSupported()) {
            RTC_LOG(LS_INFO) << "Using Java Audio module for input";
            return std::make_unique<JavaAudioDeviceModule>(desc, isCapture, sink);
        }
#endif
        throw MediaDeviceError("Unsupported platform for audio device");
    }
} // ntgcalls