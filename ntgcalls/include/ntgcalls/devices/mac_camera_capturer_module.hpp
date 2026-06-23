//
// Created by Laky64 on 23/06/2026.
//

#pragma once

#ifdef IS_MACOS
#include <wrtc/utils/json.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {
    using wrtc::json;

    class MacCameraCapturerModule final: public BaseReader {
        VideoDescription desc;
        void* capturer = nullptr;
        void* delegate = nullptr;
        void* device = nullptr;
        void* format = nullptr;

        void destroy();

        static void* captureDevices();

    public:
        MacCameraCapturerModule(const VideoDescription& desc, BaseSink* sink);

        ~MacCameraCapturerModule() override;

        static std::vector<DeviceInfo> GetSources();

        void onCapturedFrame(
            const uint8_t* dataY, int strideY,
            const uint8_t* dataU, int strideU,
            const uint8_t* dataV, int strideV,
            int width, int height, int rotation);

        void open() override;
    };

} // ntgcalls

#endif