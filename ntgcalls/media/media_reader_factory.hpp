//
// Created by Laky64 on 23/08/2023.
//

#pragma once

#include "../exceptions.hpp"
#include "../io/base_reader.hpp"
#include "../io/file_reader.hpp"
#include "../io/shell_reader.hpp"
#include "../models/media_description.hpp"

namespace ntgcalls {

    class MediaReaderFactory {
    private:
        std::shared_ptr<BaseReader> fromInput(BaseMediaDescription desc);

    public:
        MediaReaderFactory(MediaDescription desc);

        ~MediaReaderFactory();

        std::shared_ptr<BaseReader> audio, video;
    };

} // ntgcalls

