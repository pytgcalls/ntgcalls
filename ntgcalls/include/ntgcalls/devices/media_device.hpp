//
// Created by Laky64 on 17/09/24.
//

#pragma once
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {

    class MediaDevice {
        static std::unique_ptr<BaseIO> CreateAudioDevice(const AudioDescription* desc, BaseSink *sink, bool isCapture);

    public:
        template <typename T>
        static std::unique_ptr<T> CreateDevice(const BaseMediaDescription& desc, BaseSink* sink, const bool isCapture) {
            if (auto* audio = dynamic_cast<const AudioDescription*>(&desc)) {
                auto ioDevice = CreateAudioDevice(audio, sink, isCapture);
                return std::unique_ptr<T>(dynamic_cast<T*>(ioDevice.release()));
            }
            throw MediaDeviceError("Unsupported media type");
        }

        static std::vector<DeviceInfo> GetAudioDevices();

        static std::vector<DeviceInfo> GetScreenDevices();

        static std::vector<DeviceInfo> GetCameraDevices();

        static std::unique_ptr<BaseReader> CreateDesktopCapture(const VideoDescription& desc, BaseSink* sink);

        static std::unique_ptr<BaseReader> CreateCameraCapture(const VideoDescription& desc, BaseSink* sink);
    };

} // ntgcalls
