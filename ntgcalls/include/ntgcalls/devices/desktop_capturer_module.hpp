//
// Created by Laky64 on 15/10/24.
//

#pragma once

#ifndef IS_ANDROID
#include <wrtc/utils/json.hpp>
#include <rtc_base/platform_thread.h>
#include <ntgcalls/io/base_reader.hpp>
#include <wrtc/utils/sync_helper.hpp>
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/models/media_description.hpp>
#include <modules/desktop_capture/desktop_and_cursor_composer.h>
#include <modules/desktop_capture/delegated_source_list_controller.h>

namespace ntgcalls {
    using wrtc::json;

    class DesktopCapturerModule final: public BaseReader, public wrtc::SyncHelper, public webrtc::DesktopCapturer::Callback, public webrtc::DelegatedSourceListController::Observer {
        std::unique_ptr<webrtc::DesktopCapturer> capturer;
        VideoDescription desc;
        webrtc::PlatformThread thread;

        static std::unique_ptr<webrtc::DesktopCapturer> CreateCapturer();

    public:
        DesktopCapturerModule(const VideoDescription& desc, BaseSink* sink);

        ~DesktopCapturerModule() override;

        void OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame) override;

        void OnSelection() override;

        void OnCancelled() override;

        void OnError() override;

        void open() override;

        static bool IsSupported();

        static std::vector<DeviceInfo> GetSources();
    };

} // ntgcalls

#endif