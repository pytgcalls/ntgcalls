//
// Created by Lauren on 18/09/24.
//

#pragma once

#include <ntgcalls/media/media_description.hpp>
#include <wrtc/utils/json.hpp>

namespace ntgcalls::media::devices {
    using wrtc::utils::json;

    class BaseDeviceModule {
    protected:
        uint32_t rate_ = 0;
        uint8_t channels_ = 0;
        json device_metadata_;
        bool is_capture_;

    public:
        explicit BaseDeviceModule(const AudioDescription* desc, bool is_capture);

        virtual ~BaseDeviceModule() = default;
    };

} // ntgcalls::media::devices
