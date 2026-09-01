//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <ntgcalls/io/audio_writer.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/media_description.hpp>

namespace ntgcalls::media {

    class MediaSourceFactory {
    public:
        static std::unique_ptr<io::BaseReader> from_input(const BaseMediaDescription& desc, BaseSink* sink);

        static std::unique_ptr<io::AudioWriter> from_audio_output(const BaseMediaDescription& desc, BaseSink* sink);
    };

} // ntgcalls::media
