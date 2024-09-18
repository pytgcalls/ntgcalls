//
// Created by Laky64 on 17/09/24.
//

#pragma once
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {

    class MediaDevice {
        static std::unique_ptr<BaseReader> CreateAudioInput(const AudioDescription* desc, int64_t bufferSize);

    public:
        static std::unique_ptr<BaseReader> CreateInput(const BaseMediaDescription& desc, int64_t bufferSize);
    };

} // ntgcalls
