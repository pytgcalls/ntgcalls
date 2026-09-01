//
// Created by Lauren on 17/10/24.
//

#pragma once

#if !defined(IS_ANDROID) && !defined(IS_MACOS)
#include <api/video/video_frame.h>
#include <api/video/video_sink_interface.h>
#include <modules/video_capture/video_capture_factory.h>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/media_description.hpp>
#include <ntgcalls/media/devices/device_info.hpp>
#include <wrtc/utils/json.hpp>

namespace ntgcalls::media::devices {
    using wrtc::utils::json;

    class CameraCapturerModule final: public io::BaseReader, public webrtc::VideoSinkInterface<webrtc::VideoFrame> {
        VideoDescription desc_;
        webrtc::VideoCaptureCapability capability_;
        webrtc::scoped_refptr<webrtc::VideoCaptureModule> capturer_;

        static std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> create_device_info();

        void destroy();

    public:
        explicit CameraCapturerModule(const VideoDescription& desc, BaseSink* sink);

        ~CameraCapturerModule() override;

        static std::vector<DeviceInfo> get_sources();

        void OnFrame(const webrtc::VideoFrame& frame) override;

        void open() override;
    };

} // ntgcalls::media::devices

#endif
