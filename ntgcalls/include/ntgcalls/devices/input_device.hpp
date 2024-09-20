//
// Created by Laky64 on 18/09/24.
//

#pragma once

#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/devices/base_device_module.hpp>

namespace ntgcalls {

    class InputDevice final : public BaseReader {
        std::unique_ptr<BaseDeviceModule> adm;

    protected:
        bytes::unique_binary readInternal(int64_t size) override;

        void close() override;

    public:
        InputDevice(std::unique_ptr<BaseDeviceModule> adm, int64_t bufferSize);
    };

} // ntgcalls