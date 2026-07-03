//
// Created by Lauren on 17/10/24.
//

#if !defined(IS_ANDROID) && !defined(IS_MACOS)
#include <libyuv/scale.h>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/devices/camera_capturer_module.hpp>

#ifdef IS_LINUX
#include <modules/video_capture/video_capture_options.h>
#endif

namespace ntgcalls::media::devices {
    CameraCapturerModule::CameraCapturerModule(const VideoDescription& desc, BaseSink* sink): BaseIO(sink), BaseReader(sink), desc_(desc) {
        std::string device_id;
        try {
            auto source_metadata = json::parse(desc.input);
            device_id = source_metadata["id"].get<std::string>();
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
#ifdef IS_LINUX
        auto options = webrtc::VideoCaptureOptions();
        options.set_allow_v4l2(true);
        capturer_ = webrtc::VideoCaptureFactory::Create(&options, device_id.c_str());
#else
        capturer_ = webrtc::VideoCaptureFactory::Create(device_id.c_str());
#endif
        if (!capturer_) {
            throw MediaDeviceError("Failed to create video capturer");
        }
        capturer_->RegisterCaptureDataCallback(this);
        const auto info = create_device_info();
        auto requested = webrtc::VideoCaptureCapability();
        requested.videoType = webrtc::VideoType::kI420;
        requested.width = desc.width;
        requested.height = desc.height;
        requested.maxFPS = desc.fps;
        info->GetBestMatchedCapability(capturer_->CurrentDeviceName(), requested, capability_);
        if (!capability_.width || !capability_.height || !capability_.maxFPS) {
            capability_.width = desc.width;
            capability_.height = desc.height;
            capability_.maxFPS = desc.fps;
        }
#ifndef IS_WINDOWS
        capability_.videoType = webrtc::VideoType::kI420;
#endif
    }

    CameraCapturerModule::~CameraCapturerModule() {
        destroy();
    }

    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> CameraCapturerModule::create_device_info() {
#ifdef IS_LINUX
        auto options = webrtc::VideoCaptureOptions();
        options.set_allow_v4l2(true);
        return std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo>(webrtc::VideoCaptureFactory::CreateDeviceInfo(&options));
#else
        return std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo>(webrtc::VideoCaptureFactory::CreateDeviceInfo());
#endif
    }

    void CameraCapturerModule::destroy() {
        if (!capturer_) {
            return;
        }
        capturer_->StopCapture();
        capturer_->DeRegisterCaptureDataCallback();
        capturer_ = nullptr;
    }

    std::vector<DeviceInfo> CameraCapturerModule::get_sources() {
        const auto info = create_device_info();
        if (!info) {
            return {};
        }
        const auto count = info->NumberOfDevices();
        if (count <= 0) {
            return {};
        }
        std::vector<DeviceInfo> result;
        for (int i = 0; i < count; i++) {
            char id[256];
            if (char name[256]; info->GetDeviceName(i, name, sizeof(name), id, sizeof(id)) != -1) {
                const json metadata{
                    {"id", id},
                };
                result.emplace_back(name, metadata.dump());
            }
        }
        return result;
    }

    void CameraCapturerModule::OnFrame(const webrtc::VideoFrame& frame) {
        const auto y_scaled_size = desc_.width * desc_.height;
        const auto uv_scaled_size = y_scaled_size / 4;
        auto yuv = bytes::make_unique_binary(y_scaled_size + uv_scaled_size * 2);
        const auto buffer = frame.video_frame_buffer()->ToI420();

        const auto width = buffer->width();
        const auto height = buffer->height();
        const auto y_scaled_plane = std::make_unique<uint8_t[]>(y_scaled_size);
        const auto u_scaled_plane = std::make_unique<uint8_t[]>(uv_scaled_size);
        const auto v_scaled_plane = std::make_unique<uint8_t[]>(uv_scaled_size);

        I420Scale(
            buffer->DataY(), buffer->StrideY(),
            buffer->DataU(), buffer->StrideU(),
            buffer->DataV(), buffer->StrideV(),
            width, height,
            y_scaled_plane.get(), desc_.width,
            u_scaled_plane.get(), desc_.width / 2,
            v_scaled_plane.get(), desc_.width / 2,
            desc_.width, desc_.height,
            libyuv::kFilterBox
        );
        std::memcpy(yuv.get(), y_scaled_plane.get(), y_scaled_size);
        std::memcpy(yuv.get() + y_scaled_size, u_scaled_plane.get(), uv_scaled_size);
        std::memcpy(yuv.get() + y_scaled_size + uv_scaled_size, v_scaled_plane.get(), uv_scaled_size);

        (void) data_callback_(std::move(yuv), {
            0,
            frame.rotation(),
            static_cast<uint16_t>(desc_.width),
            static_cast<uint16_t>(desc_.height),
        });
    }

    void CameraCapturerModule::open() {
        if (capturer_->StartCapture(capability_) != 0) {
            destroy();
        }
    }
} // ntgcalls::media::devices

#endif