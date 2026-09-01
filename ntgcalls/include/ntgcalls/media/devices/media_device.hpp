//
// Created by Lauren on 17/09/24.
//

#pragma once
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/media_description.hpp>
#include <ntgcalls/media/devices/device_info.hpp>

namespace ntgcalls::media::devices {

    class MediaDevice {
        static std::unique_ptr<io::BaseIO> create_audio_device(const AudioDescription* desc, BaseSink* sink, bool is_capture);

    public:
        template<typename T>
        static std::unique_ptr<T> create_device(const BaseMediaDescription& desc, BaseSink* sink, const bool is_capture) {
            if (auto* audio = dynamic_cast<const AudioDescription*>(&desc)) {
                auto io_device = create_audio_device(audio, sink, is_capture);
                return std::unique_ptr<T>(dynamic_cast<T*>(io_device.release()));
            }
            throw MediaDeviceError("Unsupported media type");
        }

        static std::vector<DeviceInfo> get_audio_devices();

        static std::vector<DeviceInfo> get_screen_devices();

        static std::vector<DeviceInfo> get_camera_devices();

        static std::unique_ptr<io::BaseReader> create_desktop_capture(const VideoDescription& desc, BaseSink* sink);

        static std::unique_ptr<io::BaseReader> create_camera_capture(const VideoDescription& desc, BaseSink* sink);
    };

} // ntgcalls::media::devices
