//
// Created by Laky64 on 15/10/24.
//

#pragma once

#if !defined(IS_ANDROID) && !defined(IS_MACOS)
#include <nlohmann/json.hpp>
#include <rtc_base/platform_thread.h>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/models/media_description.hpp>
#include <modules/desktop_capture/desktop_and_cursor_composer.h>

namespace ntgcalls {
    using nlohmann::json;

    class DesktopCapturerModule final: public BaseReader, public webrtc::DesktopCapturer::Callback {
        std::unique_ptr<webrtc::DesktopCapturer> capturer;
        VideoDescription desc;
        rtc::PlatformThread thread;

        static std::unique_ptr<webrtc::DesktopCapturer> CreateCapturer();

    public:
        DesktopCapturerModule(const VideoDescription& desc, BaseSink* sink);

        ~DesktopCapturerModule() override;

        void OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame) override;

        void open() override;

        static bool IsSupported();

        static std::vector<DeviceInfo> GetSources();
    };

} // ntgcalls

#endif