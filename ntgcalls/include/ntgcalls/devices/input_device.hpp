//
// Created by Laky64 on 18/09/24.
//

#pragma once

#ifdef IS_LINUX
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/models/media_description.hpp>
#include <ntgcalls/devices/base_device_module.hpp>

namespace ntgcalls {

    class InputDevice final : public BaseReader {
        std::unique_ptr<BaseDeviceModule> adm;

    protected:
        bytes::unique_binary readInternal(int64_t size) override;

        void close() override;

    public:
        InputDevice(const AudioDescription* desc, std::unique_ptr<BaseDeviceModule> adm, int64_t bufferSize);
    };

} // alsa

#endif