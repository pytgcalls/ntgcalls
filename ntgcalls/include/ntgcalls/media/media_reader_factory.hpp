//
// Created by Laky64 on 23/08/2023.
//

#pragma once

#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {

    class MediaReaderFactory {
    public:
        static std::unique_ptr<BaseReader> fromInput(const BaseMediaDescription& desc, BaseSink *sink);
    };

} // ntgcalls

