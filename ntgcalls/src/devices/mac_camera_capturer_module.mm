//
// Created by Laky64 on 23/06/2026.
//

#include <ntgcalls/devices/mac_camera_capturer_module.hpp>

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

@interface NTgCameraCapturerDelegate : NSObject <RTCVideoCapturerDelegate>
@property(nonatomic, assign) ntgcalls::MacCameraCapturerModule* owner;
@end

@implementation NTgCameraCapturerDelegate
- (void)capturer:(RTCVideoCapturer *)capturer didCaptureVideoFrame:(RTCVideoFrame *)frame {
    if (!self.owner) {
        return;
    }
    id<RTCI420Buffer> i420 = [frame.buffer toI420];
    self.owner->onCapturedFrame(
        i420.dataY, i420.strideY,
        i420.dataU, i420.strideU,
        i420.dataV, i420.strideV,
        i420.width, i420.height,
        static_cast<int>(frame.rotation)
    );
}
@end

namespace ntgcalls {
    MacCameraCapturerModule::MacCameraCapturerModule(const VideoDescription& desc, BaseSink* sink): BaseIO(sink), BaseReader(sink), desc(desc) {
        std::string deviceId;
        try {
            auto sourceMetadata = json::parse(desc.input);
            deviceId = sourceMetadata["id"].get<std::string>();
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }

        AVCaptureDevice* selectedDevice = nil;
        for (AVCaptureDevice* dev in [RTCCameraVideoCapturer captureDevices]) {
            if ([dev.uniqueID isEqualToString:[NSString stringWithUTF8String:deviceId.c_str()]]) {
                selectedDevice = dev;
                break;
            }
        }
        if (!selectedDevice) {
            throw MediaDeviceError("Camera device not found");
        }

        AVCaptureDeviceFormat* bestFormat = nil;
        int bestDiff = INT_MAX;
        for (AVCaptureDeviceFormat* fmt in [RTCCameraVideoCapturer supportedFormatsForDevice:selectedDevice]) {
            const CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(fmt.formatDescription);
            bool fpsSupported = false;
            for (AVFrameRateRange* range in fmt.videoSupportedFrameRateRanges) {
                if (desc.fps >= range.minFrameRate && desc.fps <= range.maxFrameRate) {
                    fpsSupported = true;
                    break;
                }
            }
            if (!fpsSupported) {
                continue;
            }
            const int diff = std::abs(dimensions.width - desc.width) + std::abs(dimensions.height - desc.height);
            if (diff < bestDiff) {
                bestDiff = diff;
                bestFormat = fmt;
            }
        }
        if (!bestFormat) {
            bestFormat = selectedDevice.activeFormat;
        }
        if (!bestFormat) {
            throw MediaDeviceError("No supported camera format found");
        }

        NTgCameraCapturerDelegate* cameraDelegate = [[NTgCameraCapturerDelegate alloc] init];
        cameraDelegate.owner = this;
        RTCCameraVideoCapturer* cameraCapturer = [[RTCCameraVideoCapturer alloc] initWithDelegate:cameraDelegate];

        delegate = static_cast<void*>(cameraDelegate);
        capturer = static_cast<void*>(cameraCapturer);
        device = static_cast<void*>([selectedDevice retain]);
        format = static_cast<void*>([bestFormat retain]);
    }

    MacCameraCapturerModule::~MacCameraCapturerModule() {
        destroy();
    }

    void MacCameraCapturerModule::destroy() {
        if (capturer) {
            [static_cast<RTCCameraVideoCapturer*>(capturer) stopCapture];
        }
        if (delegate) {
            static_cast<NTgCameraCapturerDelegate*>(delegate).owner = nullptr;
        }
        if (capturer) {
            [static_cast<RTCCameraVideoCapturer*>(capturer) release];
            capturer = nullptr;
        }
        if (delegate) {
            [static_cast<NTgCameraCapturerDelegate*>(delegate) release];
            delegate = nullptr;
        }
        if (device) {
            [static_cast<AVCaptureDevice*>(device) release];
            device = nullptr;
        }
        if (format) {
            [static_cast<AVCaptureDeviceFormat*>(format) release];
            format = nullptr;
        }
    }

    std::vector<DeviceInfo> MacCameraCapturerModule::GetSources() {
        std::vector<DeviceInfo> result;
        for (AVCaptureDevice* dev in [RTCCameraVideoCapturer captureDevices]) {
            const json metadata{
                {"id", std::string(dev.uniqueID.UTF8String)},
            };
            result.emplace_back(std::string(dev.localizedName.UTF8String), metadata.dump());
        }
        return result;
    }

    void MacCameraCapturerModule::onCapturedFrame(
        const uint8_t* dataY, const int strideY,
        const uint8_t* dataU, const int strideU,
        const uint8_t* dataV, const int strideV,
        const int width, const int height, const int rotation) {
        const auto yScaledSize = desc.width * desc.height;
        const auto uvScaledSize = yScaledSize / 4;
        auto yuv = bytes::make_unique_binary(yScaledSize + uvScaledSize * 2);

        const auto yScaledPlane = std::make_unique<uint8_t[]>(yScaledSize);
        const auto uScaledPlane = std::make_unique<uint8_t[]>(uvScaledSize);
        const auto vScaledPlane = std::make_unique<uint8_t[]>(uvScaledSize);

        libyuv::I420Scale(
            dataY, strideY,
            dataU, strideU,
            dataV, strideV,
            width, height,
            yScaledPlane.get(), desc.width,
            uScaledPlane.get(), desc.width / 2,
            vScaledPlane.get(), desc.width / 2,
            desc.width, desc.height,
            libyuv::kFilterBox
        );
        memcpy(yuv.get(), yScaledPlane.get(), yScaledSize);
        memcpy(yuv.get() + yScaledSize, uScaledPlane.get(), uvScaledSize);
        memcpy(yuv.get() + yScaledSize + uvScaledSize, vScaledPlane.get(), uvScaledSize);

        (void) dataCallback(std::move(yuv), {
            0,
            static_cast<webrtc::VideoRotation>(rotation),
            static_cast<uint16_t>(desc.width),
            static_cast<uint16_t>(desc.height),
        });
    }

    void MacCameraCapturerModule::open() {
        if (running) return;
        running = true;
        [static_cast<RTCCameraVideoCapturer*>(capturer)
            startCaptureWithDevice:static_cast<AVCaptureDevice*>(device)
                            format:static_cast<AVCaptureDeviceFormat*>(format)
                               fps:desc.fps];
    }
} // ntgcalls

#endif