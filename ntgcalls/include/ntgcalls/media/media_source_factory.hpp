//
// Created by Laky64 on 07/10/24.
//

#pragma once
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/models/media_description.hpp>
#include <ntgcalls/io/audio_writer.hpp>

namespace ntgcalls {

    class MediaSourceFactory {
    public:
        static std::unique_ptr<BaseReader> fromInput(const BaseMediaDescription& desc, BaseSink *sink);

        static std::unique_ptr<AudioWriter> fromAudioOutput(const BaseMediaDescription& desc, BaseSink* sink);
    };

} // ntgcalls
