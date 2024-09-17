//
// Created by Laky64 on 17/09/24.
//

#pragma once
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {

    class MediaDevice {
    public:
        static std::unique_ptr<BaseReader> create(const BaseMediaDescription& desc, std::string deviceId, int64_t bufferSize);
    };

} // ntgcalls
