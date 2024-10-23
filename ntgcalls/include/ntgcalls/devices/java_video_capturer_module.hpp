//
// Created by Laky64 on 19/10/24.
//

#pragma once

#ifdef IS_ANDROID

#include <jni.h>
#include <nlohmann/json.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {
    using nlohmann::json;

    class JavaVideoCapturerModule final: public BaseReader {
        VideoDescription desc;
        jobject javaModule;

    public:
        explicit JavaVideoCapturerModule(bool isScreencast, const VideoDescription& desc, BaseSink* sink);

        ~JavaVideoCapturerModule() override;

        static bool IsSupported(bool isScreencast);

        static std::vector<DeviceInfo> getDevices();

        void onCapturerStopped() const;

        void onFrame(const webrtc::VideoFrame& frame);

        void open() override;
    };

} // ntgcalls

#endif