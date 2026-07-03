//
// Created by Lauren on 19/10/24.
//

#pragma once

#ifdef IS_ANDROID

#include <jni.h>
#include <api/video/video_frame.h>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/media_description.hpp>
#include <ntgcalls/media/devices/device_info.hpp>
#include <wrtc/utils/json.hpp>

namespace ntgcalls::media::devices {
    using wrtc::utils::json;

    class JavaVideoCapturerModule final: public io::BaseReader {
        VideoDescription desc_;
        jobject java_module_;

    public:
        explicit JavaVideoCapturerModule(bool is_screencast, const VideoDescription& desc, BaseSink* sink);

        ~JavaVideoCapturerModule() override;

        static bool is_supported(bool is_screencast);

        static std::vector<DeviceInfo> get_devices();

        void on_capturer_stopped() const;

        void on_frame(const webrtc::VideoFrame& frame) const;

        void open() override;
    };

} // ntgcalls::media::devices

#endif