//
// Created by Lauren on 17/09/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/devices/camera_capturer_module.hpp>
#include <ntgcalls/media/devices/desktop_capturer_module.hpp>
#include <ntgcalls/media/devices/media_device.hpp>

#ifdef IS_LINUX
#include <ntgcalls/media/devices/alsa_device_module.hpp>
#include <ntgcalls/media/devices/pulse_device_module.hpp>
#elif IS_WINDOWS
#include <ntgcalls/media/devices/win_core_device_module.hpp>
#elif IS_ANDROID
#include <ntgcalls/media/devices/java_video_capturer_module.hpp>
#include <ntgcalls/media/devices/oboe_device_module.hpp>
#elif IS_MACOS
#include <ntgcalls/media/devices/mac_audio_device_module.hpp>
#include <ntgcalls/media/devices/mac_camera_capturer_module.hpp>
#endif

namespace ntgcalls::media::devices {
    std::vector<DeviceInfo> MediaDevice::get_audio_devices() {
#ifdef IS_LINUX
        if (PulseDeviceModule::is_supported()) {
            return PulseDeviceModule::get_devices();
        }
        if (AlsaDeviceModule::is_supported()) {
            return AlsaDeviceModule::get_devices();
        }
#elif IS_WINDOWS
        if (WinCoreDeviceModule::is_supported()) {
            return WinCoreDeviceModule::get_devices();
        }
#elif IS_ANDROID
        auto append_devices = [](std::vector<DeviceInfo>& devices, const std::string& name, const bool& is_capture) {
            const json data = {
                {"is_microphone", is_capture},
            };
            devices.emplace_back(name, data.dump());
        };
        std::vector<DeviceInfo> devices;
        append_devices(devices, "default", true);
        append_devices(devices, "default", false);
        return devices;
#elif IS_MACOS
        if (MacAudioDeviceModule::is_supported()) {
            return MacAudioDeviceModule::get_devices();
        }
#endif
#ifndef IS_ANDROID
        return {};
#endif
    }

    std::vector<DeviceInfo> MediaDevice::get_screen_devices() {
#ifdef IS_ANDROID
        if (JavaVideoCapturerModule::is_supported(true)) {
            const json metadata = {
                {"id", "screen"},
            };
            return {DeviceInfo("Device Screen", metadata.dump())};
        }
#else
        if (DesktopCapturerModule::is_supported()) {
            return DesktopCapturerModule::get_sources();
        }
#endif
        return {};
    }

    std::vector<DeviceInfo> MediaDevice::get_camera_devices() {
#if !defined(IS_ANDROID) && !defined(IS_MACOS)
        return CameraCapturerModule::get_sources();
#elif IS_ANDROID
        if (JavaVideoCapturerModule::is_supported(false)) {
            return JavaVideoCapturerModule::get_devices();
        }
        return {};
#elif IS_MACOS
        return MacCameraCapturerModule::get_sources();
#else
        return {};
#endif
    }

    std::unique_ptr<io::BaseReader> MediaDevice::create_desktop_capture(const VideoDescription& desc, BaseSink* sink) {
#ifndef IS_ANDROID
        if (DesktopCapturerModule::is_supported()) {
            RTC_LOG(LS_INFO) << "Using DesktopCapturer module for input";
            return std::make_unique<DesktopCapturerModule>(desc, sink);
        }
#elif IS_ANDROID
        if (JavaVideoCapturerModule::is_supported(true)) {
            RTC_LOG(LS_INFO) << "Using AndroidVideoCapturer module for input";
            return std::make_unique<JavaVideoCapturerModule>(true, desc, sink);
        }
#endif
        throw MediaDeviceError("Unsupported platform for desktop capture");
    }

    std::unique_ptr<io::BaseReader> MediaDevice::create_camera_capture(const VideoDescription& desc, BaseSink* sink) {
#if !defined(IS_ANDROID) && !defined(IS_MACOS)
        RTC_LOG(LS_INFO) << "Using CameraCapturer module for input";
        return std::make_unique<CameraCapturerModule>(desc, sink);
#elif IS_ANDROID
        if (JavaVideoCapturerModule::is_supported(false)) {
            RTC_LOG(LS_INFO) << "Using AndroidVideoCapturer module for input";
            return std::make_unique<JavaVideoCapturerModule>(false, desc, sink);
        }
        throw MediaDeviceError("Unsupported platform for camera capture");
#elif IS_MACOS
        RTC_LOG(LS_INFO) << "Using macOS AVFoundation camera module for input";
        return std::make_unique<MacCameraCapturerModule>(desc, sink);
#else
        throw MediaDeviceError("Unsupported platform for camera capture");
#endif
    }

    std::unique_ptr<io::BaseIO> MediaDevice::create_audio_device(const AudioDescription* desc, BaseSink *sink, const bool is_capture) {
#ifdef IS_LINUX
        if (PulseDeviceModule::is_supported()) {
            RTC_LOG(LS_INFO) << "Using PulseAudio module for input";
            return std::make_unique<PulseDeviceModule>(desc, is_capture, sink);
        }
        if (AlsaDeviceModule::is_supported()) {
            RTC_LOG(LS_INFO) << "Using ALSA module for input";
            return std::make_unique<AlsaDeviceModule>(desc, is_capture, sink);
        }
#elif IS_WINDOWS
        if (WinCoreDeviceModule::is_supported()) {
            RTC_LOG(LS_INFO) << "Using Windows Core Audio module for input";
            return std::make_unique<WinCoreDeviceModule>(desc, is_capture, sink);
        }
#elif IS_ANDROID
        RTC_LOG(LS_INFO) << "Using Oboe module for input";
        return std::make_unique<OboeDeviceModule>(desc, is_capture, sink);
#elif IS_MACOS
        if (MacAudioDeviceModule::is_supported()) {
            RTC_LOG(LS_INFO) << "Using macOS AudioQueue module for input";
            return std::make_unique<MacAudioDeviceModule>(desc, is_capture, sink);
        }
#endif
        throw MediaDeviceError("Unsupported platform for audio device");
    }
} // ntgcalls::media::devices