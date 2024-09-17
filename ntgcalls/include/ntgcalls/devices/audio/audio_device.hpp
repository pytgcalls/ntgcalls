//
// Created by Laky64 on 17/09/24.
//

#pragma once
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {

    class AudioDevice: public BaseReader {
        std::string deviceId;

    protected:
        explicit AudioDevice(std::string  deviceId, int64_t bufferSize);

    public:
        static std::unique_ptr<AudioDevice> create(const AudioDescription* desc, const std::string& deviceId, int64_t bufferSize);
    };

} // ntgcalls
