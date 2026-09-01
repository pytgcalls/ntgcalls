//
// Created by Lauren on 23/06/26.
//

#ifdef IS_MACOS

#import <AVFoundation/AVFoundation.h>
#import <sdk/objc/components/capturer/RTCCameraVideoCapturer.h>
#import <sdk/objc/base/RTCVideoFrame.h>
#import <sdk/objc/base/RTCVideoFrameBuffer.h>
#import <sdk/objc/base/RTCI420Buffer.h>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <libyuv/scale.h>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/devices/mac_camera_capturer_module.hpp>

@interface NTgCameraCapturerDelegate: NSObject <RTCVideoCapturerDelegate>
@property(nonatomic, assign) ntgcalls::media::devices::MacCameraCapturerModule* owner_module;
@end

@implementation NTgCameraCapturerDelegate {
    ntgcalls::media::devices::MacCameraCapturerModule* owner_module;
}
@synthesize owner_module = owner_module;

- (void)capturer:(RTCVideoCapturer*)capturer didCaptureVideoFrame:(RTCVideoFrame*)frame {
    if (!self.owner_module) {
        return;
    }
    id<RTCI420Buffer> i420 = [frame.buffer toI420];
    self.owner_module->on_captured_frame(
        i420.dataY, i420.strideY,
        i420.dataU, i420.strideU,
        i420.dataV, i420.strideV,
        i420.width, i420.height,
        static_cast<int>(frame.rotation)
    );
}
@end

namespace ntgcalls::media::devices {
    MacCameraCapturerModule::MacCameraCapturerModule(const media::VideoDescription& desc, BaseSink* sink): BaseIO(sink), BaseReader(sink), desc_(desc) {
        std::string device_id;
        try {
            auto source_metadata = json::parse(desc.input);
            device_id = source_metadata["id"].get<std::string>();
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }

        AVCaptureDevice* selected_device = nil;
        for (AVCaptureDevice* dev in static_cast<NSArray<AVCaptureDevice*>*>(capture_devices())) {
            if ([dev.uniqueID isEqualToString:[NSString stringWithUTF8String:device_id.c_str()]]) {
                selected_device = dev;
                break;
            }
        }
        if (!selected_device) {
            throw MediaDeviceError("Camera device not found");
        }

        AVCaptureDeviceFormat* best_format = nil;
        int best_diff = INT_MAX;
        for (AVCaptureDeviceFormat* fmt in [RTCCameraVideoCapturer supportedFormatsForDevice:selected_device]) {
            const CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(fmt.formatDescription);
            bool fps_supported = false;
            for (AVFrameRateRange* range in fmt.videoSupportedFrameRateRanges) {
                if (desc.fps >= range.minFrameRate && desc.fps <= range.maxFrameRate) {
                    fps_supported = true;
                    break;
                }
            }
            if (!fps_supported) {
                continue;
            }
            const int diff = std::abs(dimensions.width - desc.width) + std::abs(dimensions.height - desc.height);
            if (diff < best_diff) {
                best_diff = diff;
                best_format = fmt;
            }
        }
        if (!best_format) {
            best_format = selected_device.activeFormat;
        }
        if (!best_format) {
            throw MediaDeviceError("No supported camera format found");
        }

        NTgCameraCapturerDelegate* const camera_delegate = [[NTgCameraCapturerDelegate alloc] init];
        camera_delegate.owner_module = this;
        RTCCameraVideoCapturer* const camera_capturer = [[RTCCameraVideoCapturer alloc] initWithDelegate:camera_delegate];

        delegate_ = static_cast<void*>(camera_delegate);
        capturer_ = static_cast<void*>(camera_capturer);
        device_ = static_cast<void*>([selected_device retain]);
        format_ = static_cast<void*>([best_format retain]);
    }

    MacCameraCapturerModule::~MacCameraCapturerModule() {
        destroy();
    }

    void MacCameraCapturerModule::destroy() {
        if (capturer_) {
            [static_cast<RTCCameraVideoCapturer*>(capturer_) stopCapture];
        }
        if (delegate_) {
            static_cast<NTgCameraCapturerDelegate*>(delegate_).owner_module = nullptr;
        }
        if (capturer_) {
            [static_cast<RTCCameraVideoCapturer*>(capturer_) release];
            capturer_ = nullptr;
        }
        if (delegate_) {
            [static_cast<NTgCameraCapturerDelegate*>(delegate_) release];
            delegate_ = nullptr;
        }
        if (device_) {
            [static_cast<AVCaptureDevice*>(device_) release];
            device_ = nullptr;
        }
        if (format_) {
            [static_cast<AVCaptureDeviceFormat*>(format_) release];
            format_ = nullptr;
        }
    }

    void* MacCameraCapturerModule::capture_devices() {
        NSArray<AVCaptureDeviceType>* const types = @[
            AVCaptureDeviceTypeBuiltInWideAngleCamera,
            AVCaptureDeviceTypeExternal,
        ];
        AVCaptureDeviceDiscoverySession* const session = [AVCaptureDeviceDiscoverySession
            discoverySessionWithDeviceTypes:types
                                  mediaType:AVMediaTypeVideo
                                   position:AVCaptureDevicePositionUnspecified];
        return static_cast<void*>(session.devices);
    }

    std::vector<DeviceInfo> MacCameraCapturerModule::get_sources() {
        std::vector<DeviceInfo> result;
        for (AVCaptureDevice* dev in static_cast<NSArray<AVCaptureDevice*>*>(capture_devices())) {
            const json metadata{
                {"id", std::string(dev.uniqueID.UTF8String)},
            };
            result.emplace_back(std::string(dev.localizedName.UTF8String), metadata.dump());
        }
        return result;
    }

    void MacCameraCapturerModule::on_captured_frame(
        const uint8_t* data_y, const int stride_y,
        const uint8_t* data_u, const int stride_u,
        const uint8_t* data_v, const int stride_v,
        const int width, const int height, const int rotation
    ) const {
        const auto y_scaled_size = desc_.width * desc_.height;
        const auto uv_scaled_size = y_scaled_size / 4;
        auto yuv = bytes::make_unique_binary(y_scaled_size + uv_scaled_size * 2);

        const auto y_scaled_plane = std::make_unique<uint8_t[]>(y_scaled_size);
        const auto u_scaled_plane = std::make_unique<uint8_t[]>(uv_scaled_size);
        const auto v_scaled_plane = std::make_unique<uint8_t[]>(uv_scaled_size);

        libyuv::I420Scale(
            data_y, stride_y,
            data_u, stride_u,
            data_v, stride_v,
            width, height,
            y_scaled_plane.get(), desc_.width,
            u_scaled_plane.get(), desc_.width / 2,
            v_scaled_plane.get(), desc_.width / 2,
            desc_.width, desc_.height,
            libyuv::kFilterBox
        );
        memcpy(yuv.get(), y_scaled_plane.get(), y_scaled_size);
        memcpy(yuv.get() + y_scaled_size, u_scaled_plane.get(), uv_scaled_size);
        memcpy(yuv.get() + y_scaled_size + uv_scaled_size, v_scaled_plane.get(), uv_scaled_size);

        (void) data_callback_(std::move(yuv), {
                                                  0,
                                                  static_cast<webrtc::VideoRotation>(rotation),
                                                  static_cast<uint16_t>(desc_.width),
                                                  static_cast<uint16_t>(desc_.height),
                                              });
    }

    void MacCameraCapturerModule::open() {
        if (running_) return;
        running_ = true;
        [static_cast<RTCCameraVideoCapturer*>(capturer_)
            startCaptureWithDevice:static_cast<AVCaptureDevice*>(device_)
                            format:static_cast<AVCaptureDeviceFormat*>(format_)
                               fps:desc_.fps];
    }
} // ntgcalls::media::devices

#endif
