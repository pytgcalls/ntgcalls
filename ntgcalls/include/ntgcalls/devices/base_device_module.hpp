//
// Created by Laky64 on 18/09/24.
//

#pragma once

#include <nlohmann/json.hpp>
#include <wrtc/utils/binary.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {
    using nlohmann::json;

    class BaseDeviceModule {
    protected:
        uint32_t rate = 0;
        uint8_t channels = 0;
        json deviceMetadata;
        bool isCapture;

    public:
        explicit BaseDeviceModule(const AudioDescription* desc, bool isCapture);

        virtual ~BaseDeviceModule() = default;

        [[nodiscard]] virtual bytes::unique_binary read(int64_t size) = 0;

        virtual void close() = 0;
    };

} // ntgcalls
