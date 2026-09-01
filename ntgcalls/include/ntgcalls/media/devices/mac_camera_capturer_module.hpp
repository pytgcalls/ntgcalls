//
// Created by Lauren on 23/06/26.
//

#pragma once

#ifdef IS_MACOS
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/media_description.hpp>
#include <ntgcalls/media/devices/device_info.hpp>
#include <wrtc/utils/json.hpp>

namespace ntgcalls::media::devices {
    using wrtc::utils::json;

    class MacCameraCapturerModule final: public io::BaseReader {
        VideoDescription desc_;
        void* capturer_ = nullptr;
        void* delegate_ = nullptr;
        void* device_ = nullptr;
        void* format_ = nullptr;

        void destroy();

        static void* capture_devices();

    public:
        MacCameraCapturerModule(const VideoDescription& desc, BaseSink* sink);

        ~MacCameraCapturerModule() override;

        static std::vector<DeviceInfo> get_sources();

        void on_captured_frame(
            const uint8_t* data_y, int stride_y,
            const uint8_t* data_u, int stride_u,
            const uint8_t* data_v, int stride_v,
            int width, int height, int rotation
        ) const;

        void open() override;
    };

} // ntgcalls::media::devices

#endif
