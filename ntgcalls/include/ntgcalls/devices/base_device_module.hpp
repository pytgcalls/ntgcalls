//
// Created by Laky64 on 18/09/24.
//

#pragma once

#include <wrtc/utils/binary.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {

    class BaseDeviceModule {
    protected:
        uint32_t rate = 0;
        uint8_t channels = 0;
        std::string deviceId;

    public:
        explicit BaseDeviceModule(const AudioDescription* desc) : rate(desc->sampleRate), channels(desc->channelCount), deviceId(desc->input) {}

        virtual ~BaseDeviceModule() = default;

        [[nodiscard]] virtual bytes::unique_binary read(int64_t size) = 0;

        virtual void close() = 0;
    };

} // ntgcalls
