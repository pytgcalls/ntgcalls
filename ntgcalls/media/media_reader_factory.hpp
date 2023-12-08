//
// Created by Laky64 on 23/08/2023.
//

#pragma once

#include "../io/base_reader.hpp"
#include "../models/media_description.hpp"

namespace ntgcalls {

    class MediaReaderFactory {
        static std::shared_ptr<BaseReader> fromInput(const BaseMediaDescription& desc);

    public:
        explicit MediaReaderFactory(const MediaDescription& desc);

        ~MediaReaderFactory();

        std::shared_ptr<BaseReader> audio, video;
    };

} // ntgcalls

