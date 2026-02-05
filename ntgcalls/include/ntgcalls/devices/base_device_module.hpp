//
// Created by Laky64 on 18/09/24.
//

#pragma once

#include <wrtc/utils/json.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {
    using wrtc::json;

    class BaseDeviceModule {
    protected:
        uint32_t rate = 0;
        uint8_t channels = 0;
        json deviceMetadata;
        bool isCapture;

    public:
        explicit BaseDeviceModule(const AudioDescription* desc, bool isCapture);

        virtual ~BaseDeviceModule() = default;
    };

} // ntgcalls
