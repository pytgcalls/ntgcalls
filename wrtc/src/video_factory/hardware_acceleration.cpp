//
// Created by Laky64 on 22/04/25.
//

#include <array>
#include <rtc_base/logging.h>
#include <wrtc/video_factory/hardware_acceleration.hpp>

namespace wrtc {
    AVPixelFormat HardwareAcceleration::GetHwFormat(AVCodecContext *context, const AVPixelFormat *formats) {
#ifdef IS_LINUX
        const auto listAccelerations = checkHardwareLibs();
#else
        constexpr auto listAccelerations = std::array{
#ifdef IS_WINDOWS
            AV_PIX_FMT_D3D11,
            AV_PIX_FMT_DXVA2_VLD,
            AV_PIX_FMT_CUDA,
#elif defined IS_MACOS
            AV_PIX_FMT_VIDEOTOOLBOX,
#endif
        };
#endif

        for (const auto format : listAccelerations) {
            RTC_LOG(LS_VERBOSE) << "Checking hardware acceleration: " << format;
            if (!checkAvFormats(formats, format)) {
                RTC_LOG(LS_VERBOSE) << "Format not supported: " << format;
                continue;
            }
            if (const auto deviceType = getHwDeviceType(format); deviceType == AV_HWDEVICE_TYPE_NONE && context->hw_device_ctx) {
                av_buffer_unref(&context->hw_device_ctx);
                RTC_LOG(LS_VERBOSE) << "Using software decoding: " << format;
            } else if (deviceType != AV_HWDEVICE_TYPE_NONE && !initHardware(context, deviceType)) {
                RTC_LOG(LS_ERROR) << "Failed to initialize hardware acceleration: " << format;
                continue;
            }
            RTC_LOG(LS_VERBOSE) << "Using hardware acceleration: " << format;
            return format;
        }

        AVPixelFormat result = AV_PIX_FMT_NONE;
        for (const AVPixelFormat *p = formats; *p != AV_PIX_FMT_NONE; p++) {
            result = *p;
        }
        return result;
    }

    bool HardwareAcceleration::checkAvFormats(const AVPixelFormat *formats, const AVPixelFormat format) {
        const AVPixelFormat *p = nullptr;
        for (p = formats; *p != AV_PIX_FMT_NONE; p++) {
            RTC_LOG(LS_VERBOSE) << "Available format: " << *p;
            if (*p == format) {
                return true;
            }
        }
        return false;
    }

    AVHWDeviceType HardwareAcceleration::getHwDeviceType(const AVPixelFormat format) {
        switch (format) {
        case AV_PIX_FMT_D3D11:
            return AV_HWDEVICE_TYPE_D3D11VA;
        case AV_PIX_FMT_DXVA2_VLD:
            return AV_HWDEVICE_TYPE_DXVA2;
        case AV_PIX_FMT_CUDA:
            return AV_HWDEVICE_TYPE_CUDA;
        case AV_PIX_FMT_VIDEOTOOLBOX:
            return AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
        case AV_PIX_FMT_VAAPI:
            return AV_HWDEVICE_TYPE_VAAPI;
        case AV_PIX_FMT_VDPAU:
            return AV_HWDEVICE_TYPE_VDPAU;
        default:
            return AV_HWDEVICE_TYPE_NONE;
        }
    }

    bool HardwareAcceleration::initHardware(AVCodecContext* context, const AVHWDeviceType type) {
        RTC_LOG(LS_VERBOSE) << "Initializing hardware acceleration: " << type;
        auto *parent = static_cast<AVCodecContext*>(context->opaque);

        AVBufferRef* hwDeviceContext = nullptr;
        const auto error = AvErrorWrap(
            av_hwdevice_ctx_create(
                &hwDeviceContext,
                type,
                nullptr,
                nullptr,
                0
            )
        );

        if (error || !hwDeviceContext) {
            RTC_LOG(LS_ERROR) << "Failed to create hardware device context: " << error.message();
            return false;
        }

        if (parent->hw_device_ctx) {
            av_buffer_unref(&parent->hw_device_ctx);
        }
        parent->hw_device_ctx = av_buffer_ref(hwDeviceContext);
        av_buffer_unref(&hwDeviceContext);
        context->hw_device_ctx = parent->hw_device_ctx;
        return true;
    }

    std::deque<AVPixelFormat> HardwareAcceleration::checkHardwareLibs() {
        auto list = std::deque{
            AV_PIX_FMT_CUDA,
        };
        if (tryLoadLibrary("libvdpau.so.1")) {
            list.push_front(AV_PIX_FMT_VDPAU);
        }
        constexpr auto vaLibs = std::array{
            "libva-drm.so.2",
            "libva-x11.so.2",
            "libva.so.2",
            "libdrm.so.2",
        };
        bool libVAAvailable = true;
        for (const auto lib : vaLibs) {
            if (!tryLoadLibrary(lib)) {
                libVAAvailable = false;
            }
        }
        if (libVAAvailable) {
            list.push_front(AV_PIX_FMT_VAAPI);
        }
        return list;
    }

    HardwareAcceleration::LibraryHandle HardwareAcceleration::tryLoadLibrary(const char* name) {
        RTC_LOG(LS_VERBOSE) << "Loading library: " << name;
        if (auto lib = LibraryHandle(dlopen(name, RTLD_LAZY))) {
            RTC_LOG(LS_VERBOSE) << "Loaded library: " << name;
            return lib;
        }
        RTC_LOG(LS_INFO) << "Failed to load library: " << name << " Error: " << dlerror();
        return nullptr;
    }

    HardwareAcceleration::AvErrorWrap::operator bool() const {
        return _code < 0;
    }

    std::string HardwareAcceleration::AvErrorWrap::message() const {
        char string[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(
            string,
            sizeof(string),
            _code
        );
        return {string};
    }
} // wrtc