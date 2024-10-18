//
// Created by Laky64 on 17/10/24.
//

#pragma once

#if !defined(IS_ANDROID) && !defined(IS_MACOS)
#include <api/video/video_frame.h>
#include <api/video/video_sink_interface.h>
#include <nlohmann/json.hpp>
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/models/media_description.hpp>
#include <modules/video_capture/video_capture_factory.h>

namespace ntgcalls {
    using nlohmann::json;

    class CameraCapturerModule final: public BaseReader, public rtc::VideoSinkInterface<webrtc::VideoFrame> {
        VideoDescription desc;
        webrtc::VideoCaptureCapability capability;
        rtc::scoped_refptr<webrtc::VideoCaptureModule> capturer;

        static std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> CreateDeviceInfo();

        void destroy();

    public:
        explicit CameraCapturerModule(const VideoDescription& desc, BaseSink* sink);

        ~CameraCapturerModule() override;

        static std::vector<DeviceInfo> GetSources();

        void OnFrame(const webrtc::VideoFrame& frame) override;

        void open() override;
    };

} // ntgcalls

#endif
