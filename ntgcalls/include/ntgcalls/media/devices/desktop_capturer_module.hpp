//
// Created by Lauren on 15/10/24.
//

#pragma once

#ifndef IS_ANDROID
#include <modules/desktop_capture/delegated_source_list_controller.h>
#include <modules/desktop_capture/desktop_and_cursor_composer.h>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/media_description.hpp>
#include <ntgcalls/media/devices/device_info.hpp>
#include <rtc_base/platform_thread.h>
#include <wrtc/utils/json.hpp>
#include <wrtc/utils/sync_helper.hpp>

namespace ntgcalls::media::devices {
    using wrtc::utils::json;

    class DesktopCapturerModule final: public io::BaseReader, public wrtc::utils::SyncHelper, public webrtc::DesktopCapturer::Callback, public webrtc::DelegatedSourceListController::Observer {
        std::unique_ptr<webrtc::DesktopCapturer> capturer_;
        VideoDescription desc_;
        webrtc::PlatformThread thread_;

        static std::unique_ptr<webrtc::DesktopCapturer> create_capturer();

    public:
        DesktopCapturerModule(const VideoDescription& desc, BaseSink* sink);

        ~DesktopCapturerModule() override;

        void OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame) override;

        void OnSelection() override;

        void OnCancelled() override;

        void OnError() override;

        void open() override;

        static bool is_supported();

        static std::vector<DeviceInfo> get_sources();
    };

} // ntgcalls::media::devices

#endif
